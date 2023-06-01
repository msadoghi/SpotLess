#include "mem_alloc.h"
#include "query.h"
#include "ycsb_query.h"
#include "ycsb.h"
#include "global.h"
#include "message.h"
#include "crypto.h"
#include <fstream>
#include <ctime>
#include <string>

std::vector<Message *> *Message::create_messages(char *buf)
{
	std::vector<Message *> *all_msgs = new std::vector<Message *>;
	char *data = buf;
	uint64_t ptr = 0;
	uint32_t dest_id;
	uint32_t return_id;
	uint32_t txn_cnt;
	COPY_VAL(dest_id, data, ptr);
	COPY_VAL(return_id, data, ptr);
	COPY_VAL(txn_cnt, data, ptr);
	assert(dest_id == g_node_id);
	assert(return_id != g_node_id);
	assert(ISCLIENTN(return_id) || ISSERVERN(return_id) || ISREPLICAN(return_id));
	while (txn_cnt > 0)
	{
		Message *msg = create_message(&data[ptr]);
		msg->return_node_id = return_id;
		ptr += msg->get_size();
		all_msgs->push_back(msg);
		--txn_cnt;
	}
	return all_msgs;
}

Message *Message::create_message(char *buf)
{
	RemReqType rtype = NO_MSG;
	uint64_t ptr = 0;
	COPY_VAL(rtype, buf, ptr);
	Message *msg = create_message(rtype);
	msg->copy_from_buf(buf);
	return msg;
}

Message *Message::create_message(TxnManager *txn, RemReqType rtype)
{
	Message *msg = create_message(rtype);
	msg->mcopy_from_txn(txn);
	msg->copy_from_txn(txn);
	// copy latency here
	msg->lat_work_queue_time = txn->txn_stats.work_queue_time_short;
	msg->lat_msg_queue_time = txn->txn_stats.msg_queue_time_short;
	msg->lat_cc_block_time = txn->txn_stats.cc_block_time_short;
	msg->lat_cc_time = txn->txn_stats.cc_time_short;
	msg->lat_process_time = txn->txn_stats.process_time_short;
	msg->lat_network_time = txn->txn_stats.lat_network_time_start;
	msg->lat_other_time = txn->txn_stats.lat_other_time_start;

	return msg;
}

#if !BANKING_SMART_CONTRACT
Message *Message::create_message(BaseQuery *query, RemReqType rtype)
{
	Message *msg = create_message(rtype);
	((YCSBClientQueryMessage *)msg)->copy_from_query(query);
	return msg;
}
#endif

Message *Message::create_message(uint64_t txn_id, RemReqType rtype)
{
	Message *msg = create_message(rtype);
	msg->txn_id = txn_id;
	return msg;
}

Message *Message::create_message(uint64_t txn_id, uint64_t batch_id, RemReqType rtype)
{
	Message *msg = create_message(rtype);
	msg->txn_id = txn_id;
	msg->batch_id = batch_id;
	return msg;
}

Message *Message::create_message(RemReqType rtype)
{
	Message *msg;
	switch (rtype)
	{
	case INIT_DONE:
		msg = new InitDoneMessage;
		break;
	case KEYEX:
		msg = new KeyExchange;
		break;
	case READY:
		msg = new ReadyServer;
		break;

#if BANKING_SMART_CONTRACT
	case BSC_MSG:
		msg = new BankingSmartContractMessage;
		break;
#else
	case CL_QRY:
	case RTXN:
	case RTXN_CONT:
		msg = new YCSBClientQueryMessage;
		msg->init();
		break;
#endif
	case CL_BATCH:
		msg = new ClientQueryBatch;
		break;
	case RDONE:
		msg = new DoneMessage;
		break;
	case CL_RSP:
		msg = new ClientResponseMessage;
		break;
	case EXECUTE_MSG:
		msg = new ExecuteMessage;
		break;

	case PBFT_CHKPT_MSG:
		msg = new CheckpointMessage;
		break;

#if CONSENSUS == HOTSTUFF
    case PVP_SYNC_MSG:
		msg = new PVPSyncMsg;
		break;
	case PVP_GENERIC_MSG:
		msg = new PVPGenericMsg;
		break;
#if SEPARATE
	case PVP_PROPOSAL_MSG:
		msg = new PVPProposalMsg;
		break;
#endif
	case PVP_ASK_MSG:
		msg = new PVPAskMsg;
		break;
	case PVP_ASK_RESPONSE_MSG:
		msg = new PVPAskResponseMsg;
		break;
#endif
	default:
		cout << "FALSE TYPE: " << rtype << "\n";
		fflush(stdout);
		assert(false);
	}
	assert(msg);
	msg->rtype = rtype;
	msg->txn_id = UINT64_MAX;
	msg->batch_id = UINT64_MAX;
	msg->return_node_id = g_node_id;
	msg->wq_time = 0;
	msg->mq_time = 0;
	msg->ntwk_time = 0;

	msg->lat_work_queue_time = 0;
	msg->lat_msg_queue_time = 0;
	msg->lat_cc_block_time = 0;
	msg->lat_cc_time = 0;
	msg->lat_process_time = 0;
	msg->lat_network_time = 0;
	msg->lat_other_time = 0;

	return msg;
}

uint64_t Message::mget_size()
{
	uint64_t size = 0;
	size += sizeof(RemReqType);
	size += sizeof(uint64_t);

	// for stats, send message queue time
	size += sizeof(uint64_t);
	size += signature.size();
	size += pubKey.size();
	size += sizeof(sigSize);
	size += sizeof(keySize);
	size += sizeof(instance_id);
	// for stats, latency
	size += sizeof(uint64_t) * 7;

	return size;
}

void Message::mcopy_from_txn(TxnManager *txn)
{
	txn_id = txn->get_txn_id();
	instance_id = txn->instance_id;
}

void Message::mcopy_to_txn(TxnManager *txn)
{
	txn->return_id = return_node_id;
	txn->instance_id = instance_id;
}

void Message::mcopy_from_buf(char *buf)
{
	uint64_t ptr = 0;
	COPY_VAL(rtype, buf, ptr);
	COPY_VAL(txn_id, buf, ptr);
	COPY_VAL(mq_time, buf, ptr);

	COPY_VAL(lat_work_queue_time, buf, ptr);
	COPY_VAL(lat_msg_queue_time, buf, ptr);
	COPY_VAL(lat_cc_block_time, buf, ptr);
	COPY_VAL(lat_cc_time, buf, ptr);
	COPY_VAL(lat_process_time, buf, ptr);
	COPY_VAL(lat_network_time, buf, ptr);
	COPY_VAL(lat_other_time, buf, ptr);
	if (IS_LOCAL(txn_id))
	{
		lat_network_time = (get_sys_clock() - lat_network_time) - lat_other_time;
	}
	else
	{
		lat_other_time = get_sys_clock();
	}
	//printf("buftot %ld: %f, %f\n",txn_id,lat_network_time,lat_other_time);

	COPY_VAL(sigSize, buf, ptr);
	COPY_VAL(keySize, buf, ptr);
	COPY_VAL(instance_id, buf, ptr);
		
	signature.pop_back();
	pubKey.pop_back();

	char v;
	for (uint64_t i = 0; i < sigSize; i++)
	{
		COPY_VAL(v, buf, ptr);
		signature += v;
	}
	for (uint64_t j = 0; j < keySize; j++)
	{
		COPY_VAL(v, buf, ptr);
		pubKey += v;
	}
}

void Message::mcopy_to_buf(char *buf)
{
	uint64_t ptr = 0;
	COPY_BUF(buf, rtype, ptr);
	COPY_BUF(buf, txn_id, ptr);
	COPY_BUF(buf, mq_time, ptr);

	COPY_BUF(buf, lat_work_queue_time, ptr);
	COPY_BUF(buf, lat_msg_queue_time, ptr);
	COPY_BUF(buf, lat_cc_block_time, ptr);
	COPY_BUF(buf, lat_cc_time, ptr);
	COPY_BUF(buf, lat_process_time, ptr);
	lat_network_time = get_sys_clock();

	//printf("mtobuf %ld: %f, %f\n",txn_id,lat_network_time,lat_other_time);
	COPY_BUF(buf, lat_network_time, ptr);
	COPY_BUF(buf, lat_other_time, ptr);

	COPY_BUF(buf, sigSize, ptr);
	COPY_BUF(buf, keySize, ptr);
	COPY_BUF(buf, instance_id, ptr);

	char v;
	for (uint64_t i = 0; i < sigSize; i++)
	{
		v = signature[i];
		COPY_BUF(buf, v, ptr);
	}
	for (uint64_t j = 0; j < keySize; j++)
	{
		v = pubKey[j];
		COPY_BUF(buf, v, ptr);
	}
}

void Message::release_message(Message *msg, uint64_t pos)
{
	switch (msg->rtype)
	{
	case INIT_DONE:
	{
		InitDoneMessage *m_msg = (InitDoneMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case KEYEX:
	{
		KeyExchange *m_msg = (KeyExchange *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case READY:
	{
		ReadyServer *m_msg = (ReadyServer *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#if BANKING_SMART_CONTRACT
	case BSC_MSG:
	{
		BankingSmartContractMessage *m_msg = (BankingSmartContractMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#else
	case CL_QRY:
	{
		YCSBClientQueryMessage *m_msg = (YCSBClientQueryMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif
	case CL_BATCH:
	{
		ClientQueryBatch *m_msg = (ClientQueryBatch *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case RDONE:
	{
		DoneMessage *m_msg = (DoneMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case CL_RSP:
	{
		ClientResponseMessage *m_msg = (ClientResponseMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case EXECUTE_MSG:
	{
		ExecuteMessage *m_msg = (ExecuteMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}

	case PBFT_CHKPT_MSG:
	{
		CheckpointMessage *m_msg = (CheckpointMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}

#if CONSENSUS == HOTSTUFF
	case PVP_SYNC_MSG:{
		PVPSyncMsg *m_msg = (PVPSyncMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case PVP_GENERIC_MSG:
#if SEPARATE
	case PVP_GENERIC_MSG_P:
#endif
	{
		PVPGenericMsg *m_msg = (PVPGenericMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#if SEPARATE
	case PVP_PROPOSAL_MSG:{
		PVPProposalMsg *m_msg = (PVPProposalMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif
	case PVP_ASK_MSG:{
		PVPAskMsg *m_msg = (PVPAskMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case PVP_ASK_RESPONSE_MSG:{
		PVPAskResponseMsg *m_msg = (PVPAskResponseMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif

	default:
	{
		cout << "POS" << pos << endl;
		printf("PP%d %lu %lu\n", msg->rtype, msg->txn_id, msg->instance_id);
		fflush(stdout);
		assert(false);
	}
	}
	msg->dest.clear();
}
/************************/

uint64_t QueryMessage::get_size()
{
	uint64_t size = Message::mget_size();
	return size;
}

void QueryMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
}

void QueryMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void QueryMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr __attribute__((unused));
	ptr = Message::mget_size();
}

void QueryMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr __attribute__((unused));
	ptr = Message::mget_size();
}

/************************/
#if BANKING_SMART_CONTRACT
void BankingSmartContractMessage::init()
{
}

BankingSmartContractMessage::BankingSmartContractMessage() {}

BankingSmartContractMessage::~BankingSmartContractMessage()
{
	release();
}

void BankingSmartContractMessage::release()
{
	ClientQueryMessage::release();
	inputs.release();
}

uint64_t BankingSmartContractMessage::get_size()
{
	uint64_t size = 0;
	size += sizeof(RemReqType);
	size += sizeof(return_node_id);
	size += sizeof(client_startts);
	size += sizeof(size_t);
	size += sizeof(uint64_t) * inputs.size();
	size += sizeof(BSCType);

	return size;
}

void BankingSmartContractMessage::copy_from_query(BaseQuery *query) {}

void BankingSmartContractMessage::copy_from_txn(TxnManager *txn) {}

void BankingSmartContractMessage::copy_to_txn(TxnManager *txn) {}

void BankingSmartContractMessage::copy_from_buf(char *buf)
{
	uint64_t ptr = 0;
	COPY_VAL(rtype, buf, ptr);
	COPY_VAL(return_node_id, buf, ptr);
	COPY_VAL(client_startts, buf, ptr);
	size_t size;
	COPY_VAL(size, buf, ptr);
	inputs.init(size);
	for (uint64_t i = 0; i < size; i++)
	{
		uint64_t input;
		COPY_VAL(input, buf, ptr);
		inputs.add(input);
	}

	COPY_VAL(type, buf, ptr);

	assert(ptr == get_size());
}

void BankingSmartContractMessage::copy_to_buf(char *buf)
{
	uint64_t ptr = 0;
	COPY_BUF(buf, rtype, ptr);
	COPY_BUF(buf, return_node_id, ptr);
	COPY_BUF(buf, client_startts, ptr);
	size_t size = inputs.size();
	COPY_BUF(buf, size, ptr);
	for (uint64_t i = 0; i < inputs.size(); i++)
	{
		uint64_t input = inputs[i];
		COPY_BUF(buf, input, ptr);
	}

	COPY_BUF(buf, type, ptr);

	assert(ptr == get_size());
}

//returns a string representation of the requests in this message
string BankingSmartContractMessage::getRequestString()
{
	string message;
	for (uint64_t i = 0; i < inputs.size(); i++)
	{
		message += std::to_string(inputs[i]);
		message += " ";
	}

	return message;
}

//returns the string that needs to be signed/verified for this message
string BankingSmartContractMessage::getString()
{
	string message = this->getRequestString();
	message += " ";
	message += to_string(this->client_startts);

	return message;
}
#else
void YCSBClientQueryMessage::init()
{
}

YCSBClientQueryMessage::YCSBClientQueryMessage() {}

YCSBClientQueryMessage::~YCSBClientQueryMessage()
{
	release();
}

void YCSBClientQueryMessage::release()
{
	ClientQueryMessage::release();
	// Freeing requests is the responsibility of txn at commit time
	if (!ISCLIENT)
	{
		for (uint64_t i = 0; i < requests.size(); i++)
		{
			DEBUG_M("YCSBClientQueryMessage::release ycsb_request free\n");
			mem_allocator.free(requests[i], sizeof(ycsb_request));
		}
	}
	requests.release();
}

uint64_t YCSBClientQueryMessage::get_size()
{
	uint64_t size = 0;
	size += sizeof(RemReqType);
	size += sizeof(client_startts);
	size += sizeof(size_t);
	size += sizeof(ycsb_request) * requests.size();
	size += sizeof(return_node);

	return size;
}

void YCSBClientQueryMessage::copy_from_query(BaseQuery *query)
{
	ClientQueryMessage::copy_from_query(query);
	requests.copy(((YCSBQuery *)(query))->requests);
}

void YCSBClientQueryMessage::copy_from_txn(TxnManager *txn)
{
	ClientQueryMessage::mcopy_from_txn(txn);
	requests.copy(((YCSBQuery *)(txn->query))->requests);
}

void YCSBClientQueryMessage::copy_to_txn(TxnManager *txn)
{
	// this only copies over the pointers, so if requests are freed, we'll lose the request data
	ClientQueryMessage::copy_to_txn(txn);

	txn->client_id = return_node;
	// Copies pointers to txn
	((YCSBQuery *)(txn->query))->requests.append(requests);
}

void YCSBClientQueryMessage::copy_from_buf(char *buf)
{
	uint64_t ptr = 0;
	COPY_VAL(rtype, buf, ptr);
	COPY_VAL(client_startts, buf, ptr);
	size_t size;
	COPY_VAL(size, buf, ptr);
	requests.init(size);
	for (uint64_t i = 0; i < size; i++)
	{
		DEBUG_M("YCSBClientQueryMessage::copy ycsb_request alloc\n");
		ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
		COPY_VAL(*req, buf, ptr);
		assert(req->key < g_synth_table_size);
		requests.add(req);
	}

	COPY_VAL(return_node, buf, ptr);

	assert(ptr == get_size());
}

void YCSBClientQueryMessage::copy_to_buf(char *buf)
{
	uint64_t ptr = 0;
	COPY_BUF(buf, rtype, ptr);
	COPY_BUF(buf, client_startts, ptr);
	size_t size = requests.size();
	COPY_BUF(buf, size, ptr);
	for (uint64_t i = 0; i < requests.size(); i++)
	{
		ycsb_request *req = requests[i];
		assert(req->key < g_synth_table_size);
		COPY_BUF(buf, *req, ptr);
	}

	COPY_BUF(buf, return_node, ptr);

	assert(ptr == get_size());
}

//returns a string representation of the requests in this message
string YCSBClientQueryMessage::getRequestString()
{
	string message;
	for (uint64_t i = 0; i < requests.size(); i++)
	{
		message += std::to_string(requests[i]->key);
		message += " ";
		message += requests[i]->value;
		message += " ";
	}

	return message;
}

//returns the string that needs to be signed/verified for this message
string YCSBClientQueryMessage::getString()
{
	string message = this->getRequestString();
	message += " ";
	message += to_string(this->client_startts);

	return message;
}

/************************/

void YCSBQueryMessage::init()
{
}

void YCSBQueryMessage::release()
{
	QueryMessage::release();
	// Freeing requests is the responsibility of txn
	/*
  for(uint64_t i = 0; i < requests.size(); i++) {
    DEBUG_M("YCSBQueryMessage::release ycsb_request free\n");
    mem_allocator.free(requests[i],sizeof(ycsb_request));
  }
*/
	requests.release();
}

uint64_t YCSBQueryMessage::get_size()
{
	uint64_t size = QueryMessage::get_size();
	size += sizeof(size_t);
	size += sizeof(ycsb_request) * requests.size();
	return size;
}

void YCSBQueryMessage::copy_from_txn(TxnManager *txn)
{
	QueryMessage::copy_from_txn(txn);
	requests.init(g_req_per_query);
	//requests.copy(((YCSBQuery*)(txn->query))->requests);
}

void YCSBQueryMessage::copy_to_txn(TxnManager *txn)
{
	QueryMessage::copy_to_txn(txn);
	//((YCSBQuery*)(txn->query))->requests.copy(requests);
	((YCSBQuery *)(txn->query))->requests.append(requests);
}

void YCSBQueryMessage::copy_from_buf(char *buf)
{
	QueryMessage::copy_from_buf(buf);
	uint64_t ptr = QueryMessage::get_size();
	size_t size;
	COPY_VAL(size, buf, ptr);
	assert(size <= g_req_per_query);
	requests.init(size);
	for (uint64_t i = 0; i < size; i++)
	{
		DEBUG_M("YCSBQueryMessage::copy ycsb_request alloc\n");
		ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
		COPY_VAL(*req, buf, ptr);
		ASSERT(req->key < g_synth_table_size);
		requests.add(req);
	}
	assert(ptr == get_size());
}

void YCSBQueryMessage::copy_to_buf(char *buf)
{
	QueryMessage::copy_to_buf(buf);
	uint64_t ptr = QueryMessage::get_size();
	size_t size = requests.size();
	COPY_BUF(buf, size, ptr);
	for (uint64_t i = 0; i < requests.size(); i++)
	{
		ycsb_request *req = requests[i];
		COPY_BUF(buf, *req, ptr);
	}
	assert(ptr == get_size());
}

/****************************************/

#endif

/************************/

void ClientQueryMessage::init()
{
	first_startts = 0;
}

void ClientQueryMessage::release()
{
	partitions.release();
	first_startts = 0;
}

uint64_t ClientQueryMessage::get_size()
{
	uint64_t size = Message::mget_size();
	size += sizeof(client_startts);
	size += sizeof(size_t);
	size += sizeof(uint64_t) * partitions.size();
	return size;
}

void ClientQueryMessage::copy_from_query(BaseQuery *query)
{
	partitions.clear();
}

void ClientQueryMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
	partitions.clear();
	client_startts = txn->client_startts;
}

void ClientQueryMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
	txn->client_startts = client_startts;
	txn->client_id = return_node_id;
}

void ClientQueryMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_VAL(client_startts, buf, ptr);
	size_t size;
	COPY_VAL(size, buf, ptr);
	partitions.init(size);
	for (uint64_t i = 0; i < size; i++)
	{
		uint64_t part;
		COPY_VAL(part, buf, ptr);
		partitions.add(part);
	}
}

void ClientQueryMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, client_startts, ptr);
	size_t size = partitions.size();
	COPY_BUF(buf, size, ptr);
	for (uint64_t i = 0; i < size; i++)
	{
		uint64_t part = partitions[i];
		COPY_BUF(buf, part, ptr);
	}
}

/************************/

uint64_t ClientResponseMessage::get_size()
{
	uint64_t size = Message::mget_size();
	size += sizeof(uint64_t);

#if CLIENT_RESPONSE_BATCH == true
	size += sizeof(uint64_t) * index.size();
	size += sizeof(uint64_t) * client_ts.size();
#else
	size += sizeof(uint64_t);
#endif
	return size;
}

void ClientResponseMessage::init()
{
	this->index.init(get_batch_size());
	this->client_ts.init(get_batch_size());
}

void ClientResponseMessage::release()
{
#if CLIENT_RESPONSE_BATCH == true
	index.release();
	client_ts.release();
#endif
}

void ClientResponseMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
#if CONSENSUS == HOTSTUFF
	view = txn->view;
#else
	view = get_current_view(txn->get_thd_id());
#endif

#if CLIENT_RESPONSE_BATCH
	this->index.add(txn->get_txn_id());
	this->client_ts.add(txn->client_startts);
#else
	client_startts = txn->client_startts;
#endif

}

void ClientResponseMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);

#if !CLIENT_RESPONSE_BATCH
	txn->client_startts = client_startts;
#endif
}

void ClientResponseMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();

#if CLIENT_RESPONSE_BATCH == true
	index.init(get_batch_size());
	uint64_t tval;
	for (uint64_t i = 0; i < get_batch_size(); i++)
	{
		COPY_VAL(tval, buf, ptr);
		index.add(tval);
	}

	client_ts.init(get_batch_size());
	for (uint64_t i = 0; i < get_batch_size(); i++)
	{
		COPY_VAL(tval, buf, ptr);
		client_ts.add(tval);
	}
#else
	COPY_VAL(client_startts, buf, ptr);
#endif

	COPY_VAL(view, buf, ptr);
	assert(ptr == get_size());
}

void ClientResponseMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr = Message::mget_size();

#if CLIENT_RESPONSE_BATCH == true
	uint64_t tval;
	for (uint64_t i = 0; i < get_batch_size(); i++)
	{
		tval = index[i];
		COPY_BUF(buf, tval, ptr);
	}

	for (uint64_t i = 0; i < get_batch_size(); i++)
	{
		tval = client_ts[i];
		COPY_BUF(buf, tval, ptr);
	}
#else
	COPY_BUF(buf, client_startts, ptr);
#endif

	COPY_BUF(buf, view, ptr);
	assert(ptr == get_size());
}

string ClientResponseMessage::getString(uint64_t sender)
{
	string message = std::to_string(sender);
	for (uint64_t i = 0; i < get_batch_size(); i++)
	{
		message += std::to_string(index[i]) + ":" +
				   std::to_string(client_ts[i]);
	}
	return message;
}

void ClientResponseMessage::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if ENABLE_ENCRYPT
	string message = getString(g_node_id);

	signingNodeNode(message, this->signature, this->pubKey, dest_node);
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

//validate message
bool ClientResponseMessage::validate()
{
#if USE_CRYPTO
	//is signature valid
	string message = getString(this->return_node_id);
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif

	//count number of accepted response messages for this transaction
	//cout << "IN: " << this->txn_id << " :: " << this->return_node_id << "\n";
	//fflush(stdout);

	return true;
}

/************************/

uint64_t DoneMessage::get_size()
{
	uint64_t size = Message::mget_size();
	return size;
}

void DoneMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
}

void DoneMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void DoneMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();
	assert(ptr == get_size());
}

void DoneMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr = Message::mget_size();
	assert(ptr == get_size());
}

/************************/

uint64_t QueryResponseMessage::get_size()
{
	uint64_t size = Message::mget_size();
	size += sizeof(RC);
	return size;
}

void QueryResponseMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
	rc = txn->get_rc();
}

void QueryResponseMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void QueryResponseMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_VAL(rc, buf, ptr);

	assert(ptr == get_size());
}

void QueryResponseMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, rc, ptr);
	assert(ptr == get_size());
}

/************************/

uint64_t InitDoneMessage::get_size()
{
	uint64_t size = Message::mget_size();
	return size;
}

void InitDoneMessage::copy_from_txn(TxnManager *txn)
{
}

void InitDoneMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void InitDoneMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
}

void InitDoneMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
}

/************************/

uint64_t ReadyServer::get_size()
{
	uint64_t size = Message::mget_size();
	return size;
}

void ReadyServer::copy_from_txn(TxnManager *txn)
{
}

void ReadyServer::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void ReadyServer::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
}

void ReadyServer::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
}

/************************/

uint64_t KeyExchange::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(uint64_t);
	size += pkey.size();
	size += sizeof(uint64_t);
#if THRESHOLD_SIGNATURE
	if(pkey == "SECP"){
		size += sizeof(secp256k1_pubkey);
	}
#endif
	return size;
}

void KeyExchange::copy_from_txn(TxnManager *txn)
{
}

void KeyExchange::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void KeyExchange::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_VAL(pkeySz, buf, ptr);
	char v;
	for (uint64_t j = 0; j < pkeySz; j++)
	{
		COPY_VAL(v, buf, ptr);
		pkey += v;
	}
	COPY_VAL(return_node, buf, ptr);
#if THRESHOLD_SIGNATURE
	if(pkey == "SECP"){
		COPY_VAL(public_share, buf, ptr);
	}
#endif
}

void KeyExchange::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, pkeySz, ptr);
	char v;
	for (uint64_t j = 0; j < pkeySz; j++)
	{
		v = pkey[j];
		COPY_BUF(buf, v, ptr);
	}
	COPY_BUF(buf, return_node, ptr);
#if THRESHOLD_SIGNATURE
	if(pkey == "SECP"){
		COPY_BUF(buf, public_share, ptr);
	}
#endif
}

#if CLIENT_BATCH

uint64_t ClientQueryBatch::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(return_node);
	size += sizeof(batch_size);

	for (uint i = 0; i < get_batch_size(); i++)
	{
		size += cqrySet[i]->get_size();
	}
	return size;
}

void ClientQueryBatch::init()
{
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
	this->cqrySet.init(get_batch_size());
}

void ClientQueryBatch::release()
{
	for (uint64_t i = 0; i < get_batch_size(); i++)
	{
		Message::release_message(cqrySet[i]);
	}
	cqrySet.release();
}

void ClientQueryBatch::copy_from_txn(TxnManager *txn)
{
	assert(0);
}

void ClientQueryBatch::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
	assert(0);
}

void ClientQueryBatch::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();
	COPY_VAL(return_node, buf, ptr);
	COPY_VAL(batch_size, buf, ptr);
	
	for (uint64_t i = 0; i < cqrySet.size(); i++)
    {
        Message::release_message(cqrySet[i]);
    }
    cqrySet.release();
	
	cqrySet.init(get_batch_size());
	for (uint i = 0; i < get_batch_size(); i++)
	{
		Message *msg = create_message(&buf[ptr]);
		ptr += msg->get_size();
#if BANKING_SMART_CONTRACT
		cqrySet.add((BankingSmartContractMessage *)msg);
#else
		cqrySet.add((YCSBClientQueryMessage *)msg);
#endif
	}

	assert(ptr == get_size());
}

void ClientQueryBatch::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, return_node, ptr);
	COPY_BUF(buf, batch_size, ptr);
	for (uint i = 0; i < get_batch_size(); i++)
	{
		cqrySet[i]->copy_to_buf(&buf[ptr]);
		ptr += cqrySet[i]->get_size();
	}

	assert(ptr == get_size());
}

string ClientQueryBatch::getString()
{
	string message = std::to_string(this->return_node);
	for (int i = 0; i < BATCH_SIZE; i++)
	{
		message += cqrySet[i]->getRequestString();
	}

	return message;
}

void ClientQueryBatch::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	string message = this->getString();
	signingClientNode(message, this->signature, this->pubKey, dest_node);

	//cout << "Message: " << message << endl;
	//cout << "Signature: " << this->signature << " :: " << signature.size() << endl;
	//fflush(stdout);

#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

bool ClientQueryBatch::validate()
{
#if USE_CRYPTO
	string message = this->getString();

	// make sure signature is valid
	if (!validateClientNode(message, this->pubKey, this->signature, this->return_node))
	{
		assert(0);
		return false;
	}
#endif
	return true;
}

#endif // Client_Batch

/**************************************************/

uint64_t BatchRequests::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(view);

	size += sizeof(uint64_t) * index.size();
	size += sizeof(uint64_t);

	size += hash.length();

	for (uint i = 0; i < get_batch_size(); i++)
	{
		size += requestMsg[i]->get_size();
	}

	size += sizeof(batch_size);

	return size;
}

void BatchRequests::add_request_msg(uint idx, Message * msg){
     if(requestMsg[idx]){
 		Message::release_message(requestMsg[idx]);
     }
 #if BANKING_SMART_CONTRACT
 	requestMsg[idx] = static_cast<BankingSmartContractMessage *>(msg);
 #else
 	requestMsg[idx] = static_cast<YCSBClientQueryMessage*>(msg);
 #endif
}

// Initialization
void BatchRequests::init(uint64_t thd_id)
{
	// Only primary should create this message
	assert(get_current_view(thd_id) == g_node_id);
	this->view = get_current_view(thd_id);
	this->index.init(get_batch_size());
	this->requestMsg.resize(get_batch_size());
}
#if BANKING_SMART_CONTRACT
void BatchRequests::copy_from_txn(TxnManager *txn, BankingSmartContractMessage *clqry)
{
	// Index of the transaction in this bacth.
	uint64_t txnid = txn->get_txn_id();
	uint64_t idx = txnid % get_batch_size();

	// TODO: Some memory is getting consumed while storing client query.
	char *bfr = (char *)malloc(clqry->get_size());
	clqry->copy_to_buf(bfr);
	Message *tmsg = Message::create_message(bfr);
	BankingSmartContractMessage *yqry = (BankingSmartContractMessage *)tmsg;
	free(bfr);

	// this->requestMsg[idx] = yqry;
	add_request_msg(idx, yqry);
	this->index.add(txnid);
}
#else
void BatchRequests::copy_from_txn(TxnManager *txn, YCSBClientQueryMessage *clqry)
{
	// Index of the transaction in this bacth.
	uint64_t txnid = txn->get_txn_id();
	uint64_t idx = txnid % get_batch_size();

	// TODO: Some memory is getting consumed while storing client query.
	char *bfr = (char *)malloc(clqry->get_size());
	clqry->copy_to_buf(bfr);
	Message *tmsg = Message::create_message(bfr);
	YCSBClientQueryMessage *yqry = (YCSBClientQueryMessage *)tmsg;
	free(bfr);

	// this->requestMsg[idx] = yqry;
	add_request_msg(idx, yqry);
	this->index.add(txnid);
}
#endif
void BatchRequests::copy_from_txn(TxnManager *txn)
{
	// Setting txn_id 2 less than the actual value.
	this->txn_id = txn->get_txn_id() - 2;
	this->batch_size = get_batch_size();

	// Storing the representative hash of the batch.
	this->hash = txn->hash;
	this->hashSize = txn->hashSize;
	// Use these lines for testing plain hash function.
	//string message = "anc_def";
	//this->hash.add(calculateHash(message));
}

void BatchRequests::release()
{
	index.release();

	for (uint64_t i = 0; i < requestMsg.size(); i++)
	{
		Message::release_message(requestMsg[i]);
	}
	requestMsg.clear();
}

void BatchRequests::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void BatchRequests::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();
	COPY_VAL(view, buf, ptr);

	uint64_t elem;
	release();
	// Initialization
	index.init(get_batch_size());
	requestMsg.resize(get_batch_size());

	for (uint i = 0; i < get_batch_size(); i++)
	{
		COPY_VAL(elem, buf, ptr);
		index.add(elem);

		Message *msg = create_message(&buf[ptr]);
		ptr += msg->get_size();
// #if BANKING_SMART_CONTRACT
// 		requestMsg[i] = (BankingSmartContractMessage *)msg;
// #else
// 		requestMsg[i] = (YCSBClientQueryMessage *)msg;
// #endif
		add_request_msg(i, msg);
	}

	COPY_VAL(hashSize, buf, ptr);
	ptr = buf_to_string(buf, ptr, hash, hashSize);

	COPY_VAL(batch_size, buf, ptr);


	assert(ptr == get_size());
}

void BatchRequests::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, view, ptr);

	uint64_t elem;
	for (uint i = 0; i < get_batch_size(); i++)
	{
		elem = index[i];
		COPY_BUF(buf, elem, ptr);

		//copy client request stored in message to buf
		requestMsg[i]->copy_to_buf(&buf[ptr]);
		ptr += requestMsg[i]->get_size();
	}

	COPY_BUF(buf, hashSize, ptr);

	char v;
	for (uint j = 0; j < hash.size(); j++)
	{
		v = hash[j];
		COPY_BUF(buf, v, ptr);
	}

	COPY_BUF(buf, batch_size, ptr);

	assert(ptr == get_size());
}

string BatchRequests::getString(uint64_t sender)
{
	string message = std::to_string(sender);
	for (uint i = 0; i < get_batch_size(); i++)
	{
		message += std::to_string(index[i]);
		message += requestMsg[i]->getRequestString();
	}
	message += hash;

	return message;
}

void BatchRequests::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	string message = getString(g_node_id);

	signingNodeNode(message, this->signature, this->pubKey, dest_node);
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

//makes sure message is valid, returns true for false
bool BatchRequests::validate(uint64_t thd_id)
{

#if USE_CRYPTO
	string message = getString(this->return_node_id);

	//cout << "Sign: " << this->signature << "\n";
	//fflush(stdout);

	//cout << "Pkey: " << this->pubKey << "\n";
	//fflush(stdout);

	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}

#endif

	// String of transactions in a batch to generate hash.
	string batchStr;
	for (uint i = 0; i < get_batch_size(); i++)
	{
		// Append string representation of this txn.
		batchStr += this->requestMsg[i]->getString();
	}

	// Is hash of request message valid
	if (this->hash != calculateHash(batchStr))
	{
		assert(0);
		return false;
	}

	//cout << "Done Hash\n";
	//fflush(stdout);

	//is the view the same as the view observed by this message
	if (this->view != get_current_view(thd_id))
	{
		cout << "this->view: " << this->view << endl;
		cout << "get_current_view: " << get_current_view(thd_id) << endl;
		cout << "this->txn_id: " << this-> txn_id << endl;
		fflush(stdout);
		assert(0);
		return false;
	}

	return true;
}

/************************************/

uint64_t ExecuteMessage::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);

	return size;
}

void ExecuteMessage::copy_from_txn(TxnManager *txn)
{
	// Constructing txn manager for one transaction less than end index.
	this->txn_id = txn->get_txn_id() - 1;
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	this->view = get_current_view(instance_id);
	this->index = txn->get_txn_id() + 1 - get_batch_size();
	this->end_index = txn->get_txn_id();
	this->batch_size = get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
}

void ExecuteMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void ExecuteMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_VAL(view, buf, ptr);
	COPY_VAL(index, buf, ptr);
	COPY_VAL(hashSize, buf, ptr);

	ptr = buf_to_string(buf, ptr, hash, hashSize);

	COPY_VAL(return_node, buf, ptr);
	COPY_VAL(end_index, buf, ptr);
	COPY_VAL(batch_size, buf, ptr);

	assert(ptr == get_size());
}

void ExecuteMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, view, ptr);
	COPY_BUF(buf, index, ptr);
	COPY_BUF(buf, hashSize, ptr);

	char v;
	for (uint64_t i = 0; i < hash.size(); i++)
	{
		v = hash[i];
		COPY_BUF(buf, v, ptr);
	}

	COPY_BUF(buf, return_node, ptr);
	COPY_BUF(buf, end_index, ptr);
	COPY_BUF(buf, batch_size, ptr);

	assert(ptr == get_size());
}

/************************************/

uint64_t CheckpointMessage::get_size()
{
	uint64_t size = Message::mget_size();
	size += sizeof(index);
	size += sizeof(return_node);
	size += sizeof(end_index);

	return size;
}

void CheckpointMessage::copy_from_txn(TxnManager *txn)
{
	this->txn_id = txn->get_txn_id() - 5;

	//index of first request executed since last chkpt.
	#if CONSENSUS == HOTSTUFF
	this->index = curr_next_index() - get_batch_size();
	#else
	this->index = curr_next_index() - txn_per_chkpt();
	#endif
	this->return_node = g_node_id;
	this->end_index = curr_next_index() - 1;

	// Now implemted in msg_queue::enqueue
	//this->sign();
}

//unused
void CheckpointMessage::copy_to_txn(TxnManager *txn)
{
	assert(0);
	Message::mcopy_to_txn(txn);
}

void CheckpointMessage::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_VAL(index, buf, ptr);
	COPY_VAL(return_node, buf, ptr);
	COPY_VAL(end_index, buf, ptr);

	assert(ptr == get_size());
}

void CheckpointMessage::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, index, ptr);
	COPY_BUF(buf, return_node, ptr);
	COPY_BUF(buf, end_index, ptr);

	assert(ptr == get_size());
}

void CheckpointMessage::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	string message = this->toString();
	signingNodeNode(message, this->signature, this->pubKey, dest_node);
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

//validate message, add to message log, and check if node has enough messages
bool CheckpointMessage::addAndValidate()
{
	if (!this->validate())
	{
		return false;
	}

	if (this->index < get_curr_chkpt())
	{
		return false;
	}

	return true;
}

string CheckpointMessage::toString()
{
	return std::to_string(this->index) + '_' +
		   std::to_string(this->end_index) + '_' +
		   std::to_string(this->return_node); //still needs digest of state
}

//is message valid
bool CheckpointMessage::validate()
{
#if USE_CRYPTO
	string message = this->toString();
	cout << "[0]" << this->signature << endl; fflush(stdout);
	//verify signature of message
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif

	return true;
}

/************************************/

//function for copying a string into the char buffer
uint64_t Message::string_to_buf(char *buf, uint64_t ptr, string str)
{
	char v;
	for (uint64_t i = 0; i < str.size(); i++)
	{
		v = str[i];
		COPY_BUF(buf, v, ptr);
	}
	return ptr;
}

//function for copying data from the buffer into a string
uint64_t Message::buf_to_string(char *buf, uint64_t ptr, string &str, uint64_t strSize)
{
	char v;
	for (uint64_t i = 0; i < strSize; i++)
	{
		COPY_VAL(v, buf, ptr);
		str += v;
	}
	return ptr;
}

// Message Creation methods.
char *create_msg_buffer(Message *msg)
{
	char *buf = (char *)malloc(msg->get_size());
	return buf;
}

Message *deep_copy_msg(char *buf, Message *msg)
{
	msg->copy_to_buf(buf);
	Message *copy_msg = Message::create_message(buf);
	return copy_msg;
}

void delete_msg_buffer(char *buf)
{
	free(buf);
}

/************************************/

#if CONSENSUS == HOTSTUFF

uint64_t PVPSyncMsg::get_size(){
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);
	size += highQC.get_size();
#if THRESHOLD_SIGNATURE
	size += sizeof(psig_share);
#endif
	size += sizeof(sig_empty);
	if(!sig_empty){
		size += sizeof(sig_share);
	}

	size += sizeof(non_vote);

	return size;
}

string PVPSyncMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);
	return signString;
}

void PVPSyncMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if MAC_SYNC
		string message2 = this->toString();	// MAC
		signingNodeNode(message2, this->signature, this->pubKey, dest_node);
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void PVPSyncMsg::digital_sign()
{
	unsigned char message[32];
	string metadata = this->hash + this->highQC.to_string();
	metadata = calculateHash(metadata);
	memcpy(message, metadata.c_str(), 32);
	assert(secp256k1_ecdsa_sign(ctx, &(this->sig_share), message, private_key, NULL, NULL));
}

void PVPSyncMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	this->txn_id = txn->get_txn_id();
	this->view = get_current_view(this->instance_id);
	this->end_index = this->txn_id;
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = this->hash.size();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
#if MAC_VERSION
	this->psig_share = txn->psig_share;
#endif
	this->highQC = txn->highQC;
	this->highQC.grand_empty = true;
	this->highQC.signature_share_map.clear();
}

void PVPSyncMsg::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();

	COPY_VAL(view, buf, ptr);
	COPY_VAL(index, buf, ptr);
	COPY_VAL(hashSize, buf, ptr);

	ptr = buf_to_string(buf, ptr, hash, hashSize);
	COPY_VAL(return_node, buf, ptr);
	COPY_VAL(end_index, buf, ptr);
	COPY_VAL(batch_size, buf, ptr);

	ptr = highQC.copy_from_buf(ptr, buf);
#if THRESHOLD_SIGNATURE
	COPY_VAL(psig_share, buf, ptr);
#endif

	COPY_VAL(sig_empty, buf, ptr);
	if(!sig_empty){
		COPY_VAL(sig_share, buf, ptr);
	}

	COPY_VAL(non_vote, buf, ptr);

	assert(ptr == get_size());
}

void PVPSyncMsg::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, view, ptr);
	COPY_BUF(buf, index, ptr);
	COPY_BUF(buf, hashSize, ptr);
	char v;
	for (uint64_t i = 0; i < hash.size(); i++)
	{
		v = hash[i];
		COPY_BUF(buf, v, ptr);
	}
	COPY_BUF(buf, return_node, ptr);

	COPY_BUF(buf, end_index, ptr);
	COPY_BUF(buf, batch_size, ptr);
	ptr = highQC.copy_to_buf(ptr, buf);
#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, psig_share, ptr);
#endif

	COPY_BUF(buf, sig_empty, ptr);
	if(!sig_empty){
		COPY_BUF(buf, sig_share, ptr);
	}

	COPY_BUF(buf, non_vote, ptr);

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool PVPSyncMsg::validate()
{
#if USE_CRYPTO

#if MAC_SYNC
		string message2 = this->toString();
		if (!validateNodeNode(message2, this->pubKey, this->signature, this->return_node_id))
		{
			assert(0);
			return false;
		}
#endif

	if(!sig_empty){
		unsigned char message[32];
		string metadata = this->hash + this->highQC.to_string();
		metadata = calculateHash(metadata);
		memcpy(message, metadata.c_str(), 32);
		assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
	}

#endif
	return true;
}

void PVPSyncMsg::release(){
	hash.clear();
 	highQC.signature_share_map.clear();
}

#if SEPARATE


uint64_t PVPProposalMsg::get_size()
{
	uint64_t size = Message::mget_size();
	size += sizeof(view);
	size += sizeof(uint64_t) * index.size();
	size += sizeof(uint64_t);
	size += hash.length();

	for (uint i = 0; i < get_batch_size(); i++)
	{
		size += requestMsg[i]->get_size();
	}
	size += sizeof(batch_size);
	return size;
}

void PVPProposalMsg::add_request_msg(uint idx, Message * msg){
     if(requestMsg[idx]){
 		Message::release_message(requestMsg[idx]);
     }
 #if BANKING_SMART_CONTRACT
 	requestMsg[idx] = static_cast<BankingSmartContractMessage *>(msg);
 #else
 	requestMsg[idx] = static_cast<YCSBClientQueryMessage*>(msg);
 #endif
 }

// Initialization
void PVPProposalMsg::init(uint64_t instance_id)
{
	// Only primary should create this message
	assert(get_view_primary(get_last_sent_view(instance_id), instance_id) == g_node_id);
	this->instance_id = instance_id;
	this->view = get_last_sent_view(instance_id);
	this->index.init(get_batch_size());
	this->requestMsg.resize(get_batch_size());
}

void PVPProposalMsg::copy_from_txn(TxnManager *txn)
{
	// Setting txn_id 2 less than the actual value.
	this->txn_id = txn->get_txn_id() - 2;
	this->batch_size = get_batch_size();

	// Storing the representative hash of the batch.
	this->hash = txn->hash;
	this->hashSize = txn->hashSize;
	// Use these lines for testing plain hash function.
	//string message = "anc_def";
	//this->hash.add(calculateHash(message));
}

#if BANKING_SMART_CONTRACT
void PVPProposalMsg::copy_from_txn(TxnManager *txn, BankingSmartContractMessage *clqry)
{
	// Index of the transaction in this bacth.
	uint64_t txnid = txn->get_txn_id();
	uint64_t idx = txnid % get_batch_size();

	// TODO: Some memory is getting consumed while storing client query.
	char *bfr = (char *)malloc(clqry->get_size());
	clqry->copy_to_buf(bfr);
	Message *tmsg = Message::create_message(bfr);
	BankingSmartContractMessage *yqry = (BankingSmartContractMessage *)tmsg;
	free(bfr);

	// this->requestMsg[idx] = yqry;
	add_request_msg(idx, yqry);
	this->index.add(txnid);
}
#else
void PVPProposalMsg::copy_from_txn(TxnManager *txn, YCSBClientQueryMessage *clqry)
{
	// Index of the transaction in this bacth.
	uint64_t txnid = txn->get_txn_id();
	uint64_t idx = txnid % get_batch_size();

	// TODO: Some memory is getting consumed while storing client query.
	char *bfr = (char *)malloc(clqry->get_size());
	clqry->copy_to_buf(bfr);
	Message *tmsg = Message::create_message(bfr);
	YCSBClientQueryMessage *yqry = (YCSBClientQueryMessage *)tmsg;
	free(bfr);

	// this->requestMsg[idx] = yqry;
	add_request_msg(idx, yqry);
	this->index.add(txnid);
}
#endif

string PVPProposalMsg::getString(uint64_t sender){
	string message = std::to_string(sender);
	for (uint i = 0; i < get_batch_size(); i++)
	{
		message += std::to_string(index[i]);
		message += requestMsg[i]->getRequestString();
	}
	message += hash;

	message += std::to_string(view);

	return message;
}

void PVPProposalMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if MAC_SYNC
		string message = getString(g_node_id);	// MAC
		signingNodeNode(message, this->signature, this->pubKey, dest_node);
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}



void PVPProposalMsg::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_VAL(view, buf, ptr);
	uint64_t elem;
	release();
	// Initialization
	index.init(get_batch_size());
	requestMsg.resize(get_batch_size());
	for (uint i = 0; i < get_batch_size(); i++)
	{

		COPY_VAL(elem, buf, ptr);
		index.add(elem);

		Message *msg = create_message(&buf[ptr]);
		ptr += msg->get_size();
		add_request_msg(i, msg);

	}
	COPY_VAL(hashSize, buf, ptr);
	ptr = buf_to_string(buf, ptr, hash, hashSize);

	COPY_VAL(batch_size, buf, ptr);

	assert(ptr == get_size());
}


void PVPProposalMsg::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);
	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, view, ptr);
	
	uint64_t elem;
	for (uint i = 0; i < get_batch_size(); i++)
	{
		elem = index[i];
		COPY_BUF(buf, elem, ptr);
		
		// if(rtype == PVP_ASK_RESPONSE_MSG){
		// 	cout << i << endl;
		// }
		// fflush(stdout);
		//copy client request stored in message to buf
		requestMsg[i]->copy_to_buf(&buf[ptr]);
		ptr += requestMsg[i]->get_size();
	}

	COPY_BUF(buf, hashSize, ptr);

	char v;
	for (uint j = 0; j < hash.size(); j++)
	{
		v = hash[j];
		COPY_BUF(buf, v, ptr);
	}

	COPY_BUF(buf, batch_size, ptr);

	assert(ptr == get_size());
}


//makes sure message is valid, returns true for false
bool PVPProposalMsg::validate(uint64_t thd_id)
{

#if USE_CRYPTO

#if MAC_SYNC
	string message = getString(this->return_node_id);
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif


#endif

	// String of transactions in a batch to generate hash.
	string batchStr;
	for (uint i = 0; i < get_batch_size(); i++)
	{
		// Append string representation of this txn.
		batchStr += this->requestMsg[i]->getString();
	}

	// Is hash of request message valid
	if (this->hash != calculateHash(batchStr))
	{
		assert(0);
		return false;
	}

	return true;
}

void PVPProposalMsg::release()
{
	index.release();
	for (uint64_t i = 0; i < requestMsg.size(); i++)
	{
		Message::release_message(requestMsg[i]);
	}
	requestMsg.clear();
	hash.clear();
}

uint64_t PVPGenericMsg::get_size()
{
	uint64_t size = Message::mget_size();
	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);

#if THRESHOLD_SIGNATURE
	size += sizeof(psig_share);
#endif
	size += highQC.get_size();
	return size;
}

string PVPGenericMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void PVPGenericMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if MAC_SYNC
		string message2 = this->toString();	// MAC
		signingNodeNode(message2, this->signature, this->pubKey, dest_node);
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

#if MAC_VERSION
void PVPGenericMsg::digital_sign()
{
	unsigned char message[32];
	string metadata = this->hash + this->highQC.to_string();
	metadata = calculateHash(metadata);
	memcpy(message, metadata.c_str(), 32);
	assert(secp256k1_ecdsa_sign(ctx, &psig_share, message, private_key, NULL, NULL));
}
#endif

void PVPGenericMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
	#if MAC_VERSION
	this->psig_share = txn->psig_share;
	#endif

}

void PVPGenericMsg::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_VAL(view, buf, ptr);
	COPY_VAL(index, buf, ptr);
	COPY_VAL(hashSize, buf, ptr);

	ptr = buf_to_string(buf, ptr, hash, hashSize);

	COPY_VAL(return_node, buf, ptr);
	COPY_VAL(end_index, buf, ptr);
	COPY_VAL(batch_size, buf, ptr);

	ptr = highQC.copy_from_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_VAL(psig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

void PVPGenericMsg::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, view, ptr);
	COPY_BUF(buf, index, ptr);
	COPY_BUF(buf, hashSize, ptr);

	char v;
	for (uint64_t i = 0; i < hash.size(); i++)
	{
		v = hash[i];
		COPY_BUF(buf, v, ptr);
	}

	COPY_BUF(buf, return_node, ptr);

	COPY_BUF(buf, end_index, ptr);
	COPY_BUF(buf, batch_size, ptr);

	ptr = highQC.copy_to_buf(ptr, buf);
	
#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, psig_share, ptr);
#endif
	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool PVPGenericMsg::validate()
{
#if USE_CRYPTO

#if MAC_SYNC
	string message2 = this->toString();
	if (!validateNodeNode(message2, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif

	unsigned char message[32];
	string metadata = this->hash + this->highQC.to_string();
	metadata = calculateHash(metadata);
	memcpy(message, metadata.c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &psig_share, message, &public_keys[return_node_id]));
#endif

	return true;
}

#if EQUIV_TEST
bool PVPGenericMsg::checkQC(){
	if(g_node_id % EQUIV_FREQ != EQ_VICTIM_ID)
		return false;
	// for(auto it = highQC.signature_share_map.begin(); it != highQC.signature_share_map.end(); it++){
	// 	cout << "????" << it->first << endl;
	// }
	
	fflush(stdout);
	assert(highQC.signature_share_map.size() >= nf);
	for(auto it = highQC.signature_share_map.begin(); it != highQC.signature_share_map.end(); it++){
		unsigned char message[32];
		string metadata = this->highQC.batch_hash + this->highQC.to_string_2();
		metadata = calculateHash(metadata);
		memcpy(message, metadata.c_str(), 32);
		assert(secp256k1_ecdsa_verify(ctx, &it->second, message, &public_keys[it->first]));
	}
	return true;
}
#endif

void PVPGenericMsg::release(){
 	hash.clear();
 	highQC.release();
}

#endif

/************************************/

uint64_t PVPAskMsg::get_size(){
	uint64_t size = Message::mget_size();
	size += sizeof(view);
	size += hash.size();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	return size;
}

string PVPAskMsg::toString()
{
	string signString = std::to_string(this->view) + this->hash;
	return signString;
}

void PVPAskMsg::sign(uint64_t dest_node)
{
	string message2 = this->toString();	// MAC
	signingNodeNode(message2, this->signature, this->pubKey, dest_node);
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}


void PVPAskMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	this->txn_id = txn->get_txn_id();
	uint64_t instance_id = this->instance_id;
	this->view = get_current_view(instance_id);
	this->hash = "";
	this->hashSize = 0;
	this->return_node = g_node_id;
}

void PVPAskMsg::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	uint64_t ptr = Message::mget_size();

	COPY_VAL(view, buf, ptr);
	COPY_VAL(hashSize, buf, ptr);

	ptr = buf_to_string(buf, ptr, hash, hashSize);
	COPY_VAL(return_node, buf, ptr);

	assert(ptr == get_size());
}

void PVPAskMsg::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, view, ptr);
	COPY_BUF(buf, hashSize, ptr);
	char v;
	for (uint64_t i = 0; i < hash.size(); i++)
	{
		v = hash[i];
		COPY_BUF(buf, v, ptr);
	}
	COPY_BUF(buf, return_node, ptr);

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool PVPAskMsg::validate()
{
	string message2 = this->toString();
	if (!validateNodeNode(message2, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
	return true;
}

void PVPAskMsg::release(){
	hash.clear();
}

void PVPAskResponseMsg::copy_from_txn_manager(TxnManager* tman){
	Message *message = Message::create_message(CL_QRY);
	YCSBClientQueryMessage* request = (YCSBClientQueryMessage*)message;
	ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
    req->key = ((YCSBQuery*)(tman->query))->requests[0]->key;
    req->value = ((YCSBQuery*)(tman->query))->requests[0]->value;
	request->requests.init(g_req_per_query);
	request->requests.add(req);
	request->return_node = tman->client_id;
	request->client_startts = tman->client_startts;
	add_request_msg(tman->get_txn_id() % get_batch_size(), request);
}

void PVPAskResponseMsg::copy_from_txn(TxnManager *txn)
{
	// Setting txn_id 2 less than the actual value.
	this->instance_id = txn->instance_id;
	this->txn_id = txn->get_txn_id() - 2;
	this->batch_size = get_batch_size();

	// Storing the representative hash of the batch.
	this->hash = txn->hash;
	this->hashSize = txn->hashSize;

	this->index.init(get_batch_size());
	for(uint64_t i = txn->get_txn_id() + 1 - get_batch_size(); i <= txn->get_txn_id(); i++){
		this->index.add(i);
	}
	this->requestMsg.resize(get_batch_size());
	this->view = txn->view;
}


vector<vector<Message *>> new_view_msgs;

#endif	//CONSENSUS == HOTSTUFF

/************************************/
