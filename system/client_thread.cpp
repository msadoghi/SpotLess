#include "global.h"
#include "thread.h"
#include "client_thread.h"
#include "query.h"
#include "ycsb_query.h"
#include "client_query.h"
#include "transport.h"
#include "client_txn.h"
#include "msg_thread.h"
#include "msg_queue.h"
#include "wl.h"
#include "message.h"
#include "ring_all_comb.h"

void ClientThread::send_key()
{
	// Send everyone the public key.
	for (uint64_t i = 0; i < g_node_cnt + g_client_node_cnt; i++)
	{
		if (i == g_node_id)
		{
			continue;
		}

#if CRYPTO_METHOD_RSA || CRYPTO_METHOD_ED25519
		Message *msg = Message::create_message(KEYEX);
		KeyExchange *keyex = (KeyExchange *)msg;
		// The four first letters of the message set the type
#if CRYPTO_METHOD_RSA
		#if PRINT_KEYEX
		cout << "Sending the key RSA: " << g_public_key.size() << endl;
		#endif
		keyex->pkey = "RSA-" + g_public_key;
#elif CRYPTO_METHOD_ED25519
		#if PRINT_KEYEX
		cout << "Sending the key ED25519: " << g_public_key.size() << endl;
		#endif
		keyex->pkey = "ED2-" + g_public_key;
#endif

		keyex->pkeySz = keyex->pkey.size();
		keyex->return_node = g_node_id;

		vector<uint64_t> dest;
		dest.push_back(i);

		msg_queue.enqueue(get_thd_id(), keyex, dest);
#endif

#if CRYPTO_METHOD_CMAC_AES
		#if PRINT_KEYEX
		cout << "Sending the key CMAC: " << cmacPrivateKeys[i].size() << endl;
		#endif
		Message *msgCMAC = Message::create_message(KEYEX);
		KeyExchange *keyexCMAC = (KeyExchange *)msgCMAC;
		keyexCMAC->pkey = "CMA-" + cmacPrivateKeys[i];

		keyexCMAC->pkeySz = keyexCMAC->pkey.size();
		keyexCMAC->return_node = g_node_id;
		//msg_queue.enqueue(get_thd_id(), keyexCMAC, i);
		msg_queue.enqueue(get_thd_id(), keyexCMAC, dest);
		dest.clear();
#endif
	}
}

void ClientThread::setup()
{

	// Increment commonVar.
	batchMTX.lock();
	commonVar++;
	batchMTX.unlock();

	if (_thd_id == 0)
	{
		while (commonVar < g_client_thread_cnt + g_client_rem_thread_cnt + g_client_send_thread_cnt)
			;

		send_init_done_to_all_nodes();
		send_key();
	}
}

/*
Change for RCC
	In the while loop in this method, the client is constantly sending client_batches to server nodes,
	if there are 3 nodes, the order of nodes that the client_batches should be: 0,1,2,0,1,2,0,1,2.....
	The variable next_node_id denotes the id of the node that the next client_batch would be sent to.
	Thus, line 143 is deleted, which would reset the value of next_node_id to 0.
	And line 309 is added, which would increase the value of next_node_id by 1.
	But this only works for normal case, in order to implement the view-change sub-protocol, our system needs to adopt some other strategies
*/


RC ClientThread::run()
{
	tsetup();
	printf("Running ClientThread %ld\n", _thd_id);

	while (true)
	{
		keyMTX.lock();
		if (keyAvail)
		{
			keyMTX.unlock();
			break;
		}
		keyMTX.unlock();
	}
#if !BANKING_SMART_CONTRACT
	BaseQuery *m_query;
#endif
	uint64_t iters = 0;
	uint32_t num_txns_sent = 0;
	int txns_sent[g_node_cnt];
	for (uint32_t i = 0; i < g_node_cnt; ++i)
		txns_sent[i] = 0;

	run_starttime = get_sys_clock();

#if CLIENT_BATCH
	uint addMore = 0;

	// Initializing first batch
	Message *mssg = Message::create_message(CL_BATCH);
	ClientQueryBatch *bmsg = (ClientQueryBatch *)mssg;
	bmsg->init();
#endif

	uint32_t next_node_id = get_client_view();
	init_next_to_send();
	while (!simulation->is_done())
	{
		heartbeat();
		progress_stats();

		int32_t inf_cnt;
		uint32_t next_node = get_next_to_send();

		uint64_t limit = MAX_TXN_IN_FLIGHT / BATCH_SIZE / NODE_CNT + ((MAX_TXN_IN_FLIGHT / BATCH_SIZE) % NODE_CNT > next_node);
		if(limit == 0)
			limit = 1;

		if(get_in_round(next_node) == limit){
			inc_next_to_send();
			continue;
		}

		next_node_id = next_node;

		// Just in case...
		if (iters == UINT64_MAX)
			iters = 0;
		
#if !CLIENT_BATCH // If client batching disable
		if ((inf_cnt = client_man.inc_inflight(next_node)) < 0)
			continue;

		m_query = client_query_queue.get_next_query(next_node, _thd_id);
		if (last_send_time > 0)
		{
			INC_STATS(get_thd_id(), cl_send_intv, get_sys_clock() - last_send_time);
		}
		last_send_time = get_sys_clock();
		assert(m_query);

		DEBUG("Client: thread %lu sending query to node: %u, %d, %f\n",
			  _thd_id, next_node_id, inf_cnt, simulation->seconds_from_start(get_sys_clock()));

		Message *msg = Message::create_message((BaseQuery *)m_query, CL_QRY);
		((ClientQueryMessage *)msg)->client_startts = get_sys_clock();

		YCSBClientQueryMessage *clqry = (YCSBClientQueryMessage *)msg;
		clqry->return_node = g_node_id;

		msg_queue.enqueue(get_thd_id(), msg, next_node_id);
		num_txns_sent++;
		txns_sent[next_node]++;
		INC_STATS(get_thd_id(), txn_sent_cnt, 1);

#else // If client batching enable
		if ((inf_cnt = client_man.inc_inflight(next_node)) < 0)
		{
			continue;
		}

#if BANKING_SMART_CONTRACT
		uint64_t source = (uint64_t)rand() % 10000;
		uint64_t dest = (uint64_t)rand() % 10000;
		uint64_t amount = (uint64_t)rand() % 10000;
		BankingSmartContractMessage *clqry = new BankingSmartContractMessage();
		clqry->rtype = BSC_MSG;
		clqry->inputs.init(!(addMore % 3) ? 3 : 2);
		clqry->type = (BSCType)(addMore % 3);
		clqry->inputs.add(amount);
		clqry->inputs.add(source);
		((ClientQueryMessage *)clqry)->client_startts = get_sys_clock();
		if (addMore % 3 == 0)
			clqry->inputs.add(dest);
		clqry->return_node_id = g_node_id;
#else
		m_query = client_query_queue.get_next_query(_thd_id);
		if (last_send_time > 0)
		{
			INC_STATS(get_thd_id(), cl_send_intv, get_sys_clock() - last_send_time);
		}
		last_send_time = get_sys_clock();
		assert(m_query);
		Message *msg = Message::create_message((BaseQuery *)m_query, CL_QRY);
		((ClientQueryMessage *)msg)->client_startts = get_sys_clock();
		YCSBClientQueryMessage *clqry = (YCSBClientQueryMessage *)msg;
		clqry->return_node = g_node_id;

#endif

		bmsg->cqrySet.add(clqry);
		addMore++;
		// Resetting and sending the message
		if (addMore == g_batch_size)
		{
			//increase the number of requests being processed
			inc_in_round(next_node);
			inc_next_to_send();
			bmsg->sign(next_node_id); // Sign the message.

			vector<uint64_t> dest;
			dest.push_back(next_node_id);
			fflush(stdout);

			msg_queue.enqueue(get_thd_id(), bmsg, dest);
			dest.clear();

			num_txns_sent += g_batch_size;
			txns_sent[next_node] += g_batch_size;
			INC_STATS(get_thd_id(), txn_sent_cnt, g_batch_size);

			printf("Client: thread %lu sending query to node: %u, %d, %f\n", _thd_id, next_node_id, inf_cnt, simulation->seconds_from_start(get_sys_clock()));
			fflush(stdout);

			mssg = Message::create_message(CL_BATCH);
			bmsg = (ClientQueryBatch *)mssg;
			bmsg->init();
			addMore = 0;
		}

#endif // Batch Enable
	}

	printf("FINISH %ld:%ld\n", _node_id, _thd_id);
	fflush(stdout);
	return FINISH;
}
