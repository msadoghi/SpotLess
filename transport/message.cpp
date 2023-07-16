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
	// if(rtype != 3){
	// 	cout << "rtype: " << rtype << endl;
	// fflush(stdout);
	// }

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
	assert(rtype == CL_QRY);
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
#if SHARPER
	case SUPER_PROPOSE:
		msg = new SuperPropose;
		break;
#endif
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
	case BATCH_REQ:
		msg = new BatchRequests;
		break;

#if VIEW_CHANGES == true
	case VIEW_CHANGE:
		msg = new ViewChangeMsg;
		break;
	case NEW_VIEW:
		msg = new NewViewMsg;
		break;
#endif

	case PBFT_CHKPT_MSG:
		msg = new CheckpointMessage;
		break;
	case PBFT_PREP_MSG:
		msg = new PBFTPrepMessage;
		break;
	case PBFT_COMMIT_MSG:
		msg = new PBFTCommitMessage;
		break;

#if RING_BFT
	case COMMIT_CERT_MSG:
		msg = new CommitCertificateMessage;
		break;
	case RING_PRE_PREPARE:
		msg = new RingBFTPrePrepare;
		break;
	case RING_COMMIT:
		msg = new RingBFTCommit;
		break;
#endif

#if CONSENSUS == HOTSTUFF
	case HOTSTUFF_PREP_MSG:
		msg = new HOTSTUFFPrepareMsg;
		break;
    case HOTSTUFF_PREP_VOTE_MSG:
		msg = new HOTSTUFFPrepareVoteMsg;
		break;
    case HOTSTUFF_PRECOMMIT_MSG:
		msg = new HOTSTUFFPreCommitMsg;
		break;
    case HOTSTUFF_PRECOMMIT_VOTE_MSG:
		msg = new HOTSTUFFPreCommitVoteMsg;
		break;
    case HOTSTUFF_COMMIT_MSG:
		msg = new HOTSTUFFCommitMsg;
		break;
    case HOTSTUFF_COMMIT_VOTE_MSG:
		msg = new HOTSTUFFCommitVoteMsg;
		break;
    case HOTSTUFF_DECIDE_MSG:
		msg = new HOTSTUFFDecideMsg;
		break;
    case HOTSTUFF_NEW_VIEW_MSG:
		msg = new HOTSTUFFNewViewMsg;
		break;
	case HOTSTUFF_GENERIC_MSG:
		msg = new HOTSTUFFGenericMsg;
		break;
	case NARWHAL_PAYLOAD_MSG:
		msg = new NarwhalPayloadMsg;
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

	// for stats, latency
	size += sizeof(uint64_t) * 7;

#if SHARPER
	size += sizeof(is_cross_shard);
#endif
	return size;
}

void Message::mcopy_from_txn(TxnManager *txn)
{
	txn_id = txn->get_txn_id();
}

void Message::mcopy_to_txn(TxnManager *txn)
{
	txn->return_id = return_node_id;
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

#if SHARPER
	COPY_VAL(is_cross_shard, buf, ptr);
#endif
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
#if SHARPER
	COPY_BUF(buf, is_cross_shard, ptr);
#endif
}

void Message::release_message(Message *msg)
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
#if SHARPER
	case SUPER_PROPOSE:
	{
		SuperPropose *m_msg = (SuperPropose *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif
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
	case BATCH_REQ:
	{
		BatchRequests *m_msg = (BatchRequests *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}

#if VIEW_CHANGES == true
	case VIEW_CHANGE:
	{
		ViewChangeMsg *m_msg = (ViewChangeMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case NEW_VIEW:
	{
		NewViewMsg *m_msg = (NewViewMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif

	case PBFT_CHKPT_MSG:
	{
		CheckpointMessage *m_msg = (CheckpointMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case PBFT_PREP_MSG:
	{
		PBFTPrepMessage *m_msg = (PBFTPrepMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case PBFT_COMMIT_MSG:
	{
		PBFTCommitMessage *m_msg = (PBFTCommitMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}

#if RING_BFT
	case COMMIT_CERT_MSG:
	{
		CommitCertificateMessage *m_msg = (CommitCertificateMessage *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case RING_PRE_PREPARE:
	{
		RingBFTPrePrepare *m_msg = (RingBFTPrePrepare *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case RING_COMMIT:
	{
		RingBFTCommit *m_msg = (RingBFTCommit *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif
#if CONSENSUS == HOTSTUFF
	case HOTSTUFF_PREP_MSG:{
		HOTSTUFFPrepareMsg *m_msg = (HOTSTUFFPrepareMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_PREP_VOTE_MSG:{
		HOTSTUFFPrepareVoteMsg *m_msg = (HOTSTUFFPrepareVoteMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_PRECOMMIT_MSG:{
		HOTSTUFFPreCommitMsg *m_msg = (HOTSTUFFPreCommitMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_PRECOMMIT_VOTE_MSG:{
		HOTSTUFFPreCommitVoteMsg *m_msg = (HOTSTUFFPreCommitVoteMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_COMMIT_MSG:{
		HOTSTUFFCommitMsg *m_msg = (HOTSTUFFCommitMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_COMMIT_VOTE_MSG:{
		HOTSTUFFCommitVoteMsg *m_msg = (HOTSTUFFCommitVoteMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_DECIDE_MSG:{
		HOTSTUFFDecideMsg *m_msg = (HOTSTUFFDecideMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_NEW_VIEW_MSG:{
		HOTSTUFFNewViewMsg *m_msg = (HOTSTUFFNewViewMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case HOTSTUFF_GENERIC_MSG:{
		HOTSTUFFGenericMsg *m_msg = (HOTSTUFFGenericMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
	case NARWHAL_PAYLOAD_MSG:{
		NarwhalPayloadMsg *m_msg = (NarwhalPayloadMsg *)msg;
		m_msg->release();
		delete m_msg;
		break;
	}
#endif

	default:
	{
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
#if RING_BFT || SHARPER
	size += sizeof(is_cross_shard);
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
	#if !PVP
		hash_QC_lock.lock();
		#if CHAINED
		view = hash_to_view[txn->get_hash()];
		#else
		view = hash_to_QC[txn->get_hash()].viewNumber;
		#endif
		hash_QC_lock.unlock();
	#else
		uint64_t instance_id = txn->get_txn_id() / get_batch_size() % get_totInstances();
		hash_QC_lock[instance_id].lock();
		#if CHAINED
		view = hash_to_view[instance_id][txn->get_hash()];
		#else
		view = hash_to_QC[instance_id][txn->get_hash()].viewNumber;
		#endif
		hash_QC_lock[instance_id].unlock();
	#endif
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
#if RING_BFT || SHARPER
	COPY_VAL(is_cross_shard, buf, ptr);
#endif
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
#if RING_BFT || SHARPER
	COPY_BUF(buf, is_cross_shard, ptr);
#endif
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
	string message = getString(g_node_id);

	signingNodeNode(message, this->signature, this->pubKey, dest_node);
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
#if SHARPER
	size += sizeof(bool) * (g_shard_cnt);
#endif

	for (uint i = 0; i < get_batch_size(); i++)
	{
		size += cqrySet[i]->get_size();
	}

#if RING_BFT
	size += sizeof(is_cross_shard);
	size += sizeof(bool) * (g_shard_cnt);
#endif
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
#if SHARPER
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_VAL(involved_shards[i], buf, ptr);
	}
#endif
	
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

#if RING_BFT
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_VAL(involved_shards[i], buf, ptr);
	}
	COPY_VAL(is_cross_shard, buf, ptr);
#endif

	assert(ptr == get_size());
}

void ClientQueryBatch::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, return_node, ptr);
	COPY_BUF(buf, batch_size, ptr);
#if SHARPER
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_BUF(buf, involved_shards[i], ptr);
	}
#endif
	for (uint i = 0; i < get_batch_size(); i++)
	{
		cqrySet[i]->copy_to_buf(&buf[ptr]);
		ptr += cqrySet[i]->get_size();
	}

#if RING_BFT
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_BUF(buf, involved_shards[i], ptr);
	}
	COPY_BUF(buf, is_cross_shard, ptr);
#endif
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

#if VIEW_CHANGES
	// =====================
	// Sign Bug for forwarded messages
	uint64_t source_node_id = this->return_node;
	if (this->return_node_id < g_node_cnt)
		source_node_id = this->return_node_id;
	// =====================
	if (!validateClientNode(message, this->pubKey, this->signature, source_node_id))
	{
		assert(0);
		return false;
	}
#else
	// make sure signature is valid
	if (!validateClientNode(message, this->pubKey, this->signature, this->return_node))
	{
		assert(0);
		return false;
	}
#endif
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
#if SHARPER
	size += sizeof(bool) * (g_shard_cnt);
#endif
	size += hash.length();

	for (uint i = 0; i < get_batch_size(); i++)
	{
		size += requestMsg[i]->get_size();
	}

	size += sizeof(batch_size);

#if RING_BFT
	size += sizeof(is_cross_shard);
	size += sizeof(bool) * (g_shard_cnt);
#endif
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
#if MULTI_ON
	// Whichever replica sends this message, adds itself as the view.
	/*
	Change for RCC
	the view of pre-prepare msgs is always 0, similarly, only for normal case.
	*/
	this->view = 0;
#elif SHARPER
	assert(view_to_primary(get_current_view(thd_id)) == g_node_id);
	this->view = get_current_view(thd_id);
#elif RING_BFT
	assert(view_to_primary(get_current_view(thd_id)) == g_node_id);
	this->view = get_current_view(thd_id);
#else
	// Only primary should create this message
	assert(get_current_view(thd_id) == g_node_id);
	this->view = get_current_view(thd_id);
#endif
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
#if SHARPER
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_VAL(involved_shards[i], buf, ptr);
	}
#endif

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

#if RING_BFT
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_VAL(involved_shards[i], buf, ptr);
	}
	COPY_VAL(is_cross_shard, buf, ptr);
#endif
	assert(ptr == get_size());
}

void BatchRequests::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();
	COPY_BUF(buf, view, ptr);
#if SHARPER
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_BUF(buf, involved_shards[i], ptr);
	}
#endif

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

#if RING_BFT
	for (uint64_t i = 0; i < g_shard_cnt; i++)
	{
		COPY_BUF(buf, involved_shards[i], ptr);
	}
	COPY_BUF(buf, is_cross_shard, ptr);
#endif
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
#if !RBFT_ON
	if (this->view != get_current_view(thd_id))
	{
		cout << "this->view: " << this->view << endl;
		cout << "get_current_view: " << get_current_view(thd_id) << endl;
		cout << "this->txn_id: " << this-> txn_id << endl;
		fflush(stdout);
		assert(0);
		return false;
	}
#endif

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
	#if !PVP
	this->view = get_current_view(txn->get_thd_id());
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	this->view = get_current_view(instance_id);
	#endif
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

/**************************/

uint64_t PBFTPrepMessage::get_size()
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

void PBFTPrepMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
	this->view = get_current_view(txn->get_thd_id());
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
}

void PBFTPrepMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void PBFTPrepMessage::copy_from_buf(char *buf)
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

void PBFTPrepMessage::copy_to_buf(char *buf)
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

string PBFTPrepMessage::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void PBFTPrepMessage::sign(uint64_t dest_node)
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

//makes sure message is valid, returns true or false;
bool PBFTPrepMessage::validate()
{
#if USE_CRYPTO
	//verifies message signature
	string message = this->toString();
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif

	return true;
}

/****************************************/

uint64_t PBFTCommitMessage::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);

#if RING_BFT
	size += sizeof(is_cross_shard);
#endif
	return size;
}

void PBFTCommitMessage::copy_from_txn(TxnManager *txn)
{
	Message::mcopy_from_txn(txn);
	this->view = get_current_view(txn->get_thd_id());
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
}

void PBFTCommitMessage::copy_to_txn(TxnManager *txn)
{
	Message::mcopy_to_txn(txn);
}

void PBFTCommitMessage::copy_from_buf(char *buf)
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
#if RING_BFT
	COPY_VAL(is_cross_shard, buf, ptr);
#endif

	assert(ptr == get_size());
}

void PBFTCommitMessage::copy_to_buf(char *buf)
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

#if RING_BFT
	COPY_BUF(buf, is_cross_shard, ptr);
#endif
	assert(ptr == get_size());
}

string PBFTCommitMessage::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

//signs current message
void PBFTCommitMessage::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	string message = this->toString();

	//cout << "Signing Commit msg: " << message << endl;
#if RING_BFT
	if (this->is_cross_shard)
		signingClientNode(message, this->signature, this->pubKey, dest_node);
	else
		signingNodeNode(message, this->signature, this->pubKey, dest_node);
#else
	signingNodeNode(message, this->signature, this->pubKey, dest_node);
#endif

#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

//makes sure message is valid, returns true or false;
bool PBFTCommitMessage::validate()
{
	string message = this->toString();

#if USE_CRYPTO

//verify signature of message
#if RING_BFT
	if (this->is_cross_shard)
	{
		if (!validateClientNode(message, this->pubKey, this->signature, this->return_node_id))
		{
			assert(0);
			return false;
		}
	}
	else
	{
		if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
		{
			assert(0);
			return false;
		}
	}

#else
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif
#endif
	return true;
}

/****************************************/
/*	VIEW CHANGE SPECIFIC		*/
/****************************************/

#if VIEW_CHANGES

uint64_t ViewChangeMsg::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += sizeof(numPreMsgs);
	size += sizeof(numPrepMsgs);

	for (uint64_t i = 0; i < preMsg.size(); i++)
	{
		size += preMsg[i]->get_size();
	}

	size += sizeof(uint64_t) * numPrepMsgs * 3;
	for (uint64_t i = 0; i < numPrepMsgs; i++)
	{
		size += prepHash[i].length();
	}

	size += sizeof(return_node);

	return size;
}

void ViewChangeMsg::init(uint64_t thd_id, TxnManager *txn)
{
	// Set the succeeding replica as the new primary.
	this->view = ((get_current_view(thd_id) + 1) % g_node_cnt);
	this->index = txn->get_txn_id(); // Last checkpoint idx.
	this->return_node = g_node_id;

	// cout << "Checkpoint: " << this->index << "\n";
	fflush(stdout);

	// Start collecting from the next batch, but as curr_next_index would
	// only point to the last request in the batch, so we add required amt
	uint64_t st = get_commit_message_txn_id(this->index + get_batch_size());

	bool found;
	uint64_t j = 0;
	BatchRequests *breq;
	for (uint64_t i = st; i <= curr_next_index(); i += get_batch_size())
	{
		found = false;

		// Linearly accessing the breqStore. As all requests are stored
		// sequentially so no need for repetitive access.
		bstoreMTX.lock();
		for (; j < breqStore.size(); j++)
		{
			breq = breqStore[j];
			if (breq->index[get_batch_size() - 1] == i)
			{
				found = true;
				break;
			}
		}
		bstoreMTX.unlock();

		// Storing the matched batch to the vector.
		if (found)
		{
			char *buf = create_msg_buffer(breq);
			Message *deepCMsg = deep_copy_msg(buf, breq);
			this->preMsg.push_back((BatchRequests *)deepCMsg);
			delete_msg_buffer(buf);
		}
	}

	// Store the corresponding PBFTPrepMessage.
	for (uint64_t i = st; i <= curr_next_index(); i += get_batch_size())
	{
		TxnManager *tman = txn_table.get_transaction_manager(thd_id, i, 0);
		while (true)
		{
			bool ready = tman->unset_ready();
			if (!ready)
			{
				printf("trying to get txn_man %ld\n", tman->get_txn_id());
				continue;
			}
			else
			{
				break;
			}
		}
		prepView.push_back(get_current_view(thd_id));
		prepIdx.push_back(i);
		prepHash.push_back(tman->get_hash());
		prepHsize.push_back(tman->get_hashSize());
		bool ready = tman->set_ready();
        assert(ready);
	}

	this->numPreMsgs = this->preMsg.size();
	this->numPrepMsgs = this->prepIdx.size();
}

void ViewChangeMsg::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_VAL(view, buf, ptr);
	COPY_VAL(index, buf, ptr);
	COPY_VAL(numPreMsgs, buf, ptr);
	COPY_VAL(numPrepMsgs, buf, ptr);

	for (uint i = 0; i < numPreMsgs; i++)
	{
		Message *msg = create_message(&buf[ptr]);
		ptr += msg->get_size();
		preMsg.push_back((BatchRequests *)msg);
	}

	for (uint64_t i = 0; i < numPrepMsgs; i++)
	{
		uint64_t tval;
		string hsh;
		COPY_VAL(tval, buf, ptr);
		prepView.push_back(tval);

		COPY_VAL(tval, buf, ptr);
		prepIdx.push_back(tval);

		COPY_VAL(tval, buf, ptr);
		prepHsize.push_back(tval);

		ptr = buf_to_string(buf, ptr, hsh, tval);
		prepHash.push_back(hsh);
	}

	COPY_VAL(return_node, buf, ptr);

	assert(ptr == get_size());
}

void ViewChangeMsg::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, view, ptr);
	COPY_BUF(buf, index, ptr);
	COPY_BUF(buf, numPreMsgs, ptr);
	COPY_BUF(buf, numPrepMsgs, ptr);

	assert(numPreMsgs == preMsg.size());
	for (uint64_t i = 0; i < numPreMsgs; i++)
	{
		preMsg[i]->copy_to_buf(&buf[ptr]);
		ptr += preMsg[i]->get_size();
	}

	assert(numPrepMsgs == prepIdx.size());
	uint64_t tval;
	for (uint64_t i = 0; i < numPrepMsgs; i++)
	{
		tval = prepView[i];
		COPY_BUF(buf, tval, ptr);

		tval = prepIdx[i];
		COPY_BUF(buf, tval, ptr);

		tval = prepHsize[i];
		COPY_BUF(buf, tval, ptr);

		char v;
		string hsh = prepHash[i];
		for (uint64_t j = 0; j < tval; j++)
		{
			v = hsh[j];
			COPY_BUF(buf, v, ptr);
		}
	}

	COPY_BUF(buf, return_node, ptr);
	assert(ptr == get_size());
}

void ViewChangeMsg::release()
{
	BatchRequests *pmsg;
	uint64_t i = 0;
	for (; i < preMsg.size();)
	{
		pmsg = preMsg[i];
		preMsg.erase(preMsg.begin() + i);
		Message::release_message(pmsg);
	}
	preMsg.clear();

	prepView.clear();
	prepIdx.clear();
	prepHash.clear();
	prepHsize.clear();
}

void ViewChangeMsg::sign(uint64_t dest)
{
#if USE_CRYPTO
	string message = this->toString();

	signingNodeNode(message, this->signature, this->pubKey, dest);
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

// validate message and check to see if it should be added to message log
// and if we have recieved enough messages
bool ViewChangeMsg::addAndValidate(uint64_t thd_id)
{
	if (this->return_node != g_node_id)
	{
		if (!this->validate(thd_id))
		{
			return false;
		}
	}

	//count number of matching view change messages recieved previously
	uint64_t i = 0;
	ViewChangeMsg *vmsg;
	for (; i < view_change_msgs.size(); i++)
	{
		vmsg = view_change_msgs[i];
		if (this->return_node == vmsg->return_node ||
			this->index != vmsg->index ||
			this->view != vmsg->view)
		{
			assert(0);
		}
	}

	storeVCMsg(this);

	// If equal to the number of msgs recieved.
	i++;
	if (i < (2 * g_min_invalid_nodes + 1))
	{
		return false;
	}

	return true;
}

//validate message and all messages this message contains
bool ViewChangeMsg::validate(uint64_t thd_id)
{
	string message = this->toString();

#if USE_CRYPTO
	//verify signature of message
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}
#endif

	// Validate the prepare messages
	BatchRequests *bmsg;
	uint64_t count;
	for (uint64_t i = 0; i < preMsg.size(); i++)
	{
		bmsg = preMsg[i];
		/*
		if (!bmsg->validate(thd_id))
		{
			assert(0);
			return false;
		}
		*/

		count = 0;
		for (uint64_t j = 0; j < prepIdx.size(); j++)
		{
			if (prepIdx[j] == bmsg->txn_id + 2)
			{
				if (prepHash[j] != bmsg->hash)
				{
					assert(0);
				}
				count++;
			}
		}

		if (count == 0)
		{
			assert(0);
		}
	}

	return true;
}

string ViewChangeMsg::toString()
{
	string message;
	message += to_string(view) + '_' + to_string(index) + '_';

	for (uint i = 0; i < preMsg.size(); i++)
	{
		message += preMsg[i]->getString(this->return_node) + '_';
	}

	for (uint64_t i = 0; i < prepIdx.size(); i++)
	{
		message += to_string(prepView[i]) + '_' +
				   to_string(prepIdx[i]) + '_' +
				   prepHash[i] + '_' + to_string(prepHsize[i]);
	}

	message += to_string(return_node);

	return message;
}

/************************************/

void NewViewMsg::init(uint64_t thd_id)
{
	this->view = g_node_id;

	//add view change messages
	ViewChangeMsg *vmsg;
	for (uint64_t i = 0; i < view_change_msgs.size(); i++)
	{
		vmsg = view_change_msgs[i];

		// cout << "View MSG: " << vmsg->index << "\n";
		// fflush(stdout);

		char *buf = create_msg_buffer(vmsg);
		Message *deepCMsg = deep_copy_msg(buf, vmsg);
		deepCMsg->return_node_id = vmsg->return_node_id;
		this->viewMsg.push_back((ViewChangeMsg *)deepCMsg);
		delete_msg_buffer(buf);
	}

	this->numViewChangeMsgs = this->viewMsg.size();

	// Ideally, we should find the range of completed pre-prepare requests.
	// However, in our case as all the view change messages are same, so
	// we just copy all pre-prepare messages from one of the view change.
	vmsg = view_change_msgs[0];
	BatchRequests *pmsg;
	for (uint64_t i = 0; i < vmsg->preMsg.size(); i++)
	{
		pmsg = vmsg->preMsg[i];
		char *buf = create_msg_buffer(pmsg);
		Message *deepCMsg = deep_copy_msg(buf, pmsg);
		deepCMsg->return_node_id = pmsg->return_node_id;
		this->preMsg.push_back((BatchRequests *)deepCMsg);
		delete_msg_buffer(buf);
	}

	this->numPreMsgs = vmsg->preMsg.size();
}

uint64_t NewViewMsg::get_size()
{
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(numViewChangeMsgs);
	size += sizeof(numPreMsgs);

	for (uint64_t i = 0; i < viewMsg.size(); i++)
		size += viewMsg[i]->get_size();

	for (uint64_t i = 0; i < preMsg.size(); i++)
		size += preMsg[i]->get_size();

	return size;
}

void NewViewMsg::copy_from_buf(char *buf)
{

	Message::mcopy_from_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_VAL(view, buf, ptr);
	COPY_VAL(numViewChangeMsgs, buf, ptr);
	COPY_VAL(numPreMsgs, buf, ptr);

	for (uint i = 0; i < numViewChangeMsgs; i++)
	{
		Message *msg = create_message(&buf[ptr]);
		ptr += msg->get_size();
		viewMsg.push_back((ViewChangeMsg *)msg);
	}

	for (uint i = 0; i < numPreMsgs; i++)
	{
		Message *msg = create_message(&buf[ptr]);
		ptr += msg->get_size();
		preMsg.push_back((BatchRequests *)msg);
	}

	assert(ptr == get_size());
}

void NewViewMsg::copy_to_buf(char *buf)
{
	Message::mcopy_to_buf(buf);

	uint64_t ptr = Message::mget_size();

	COPY_BUF(buf, view, ptr);
	COPY_BUF(buf, numViewChangeMsgs, ptr);
	COPY_BUF(buf, numPreMsgs, ptr);

	assert(viewMsg.size() == numViewChangeMsgs);
	for (uint64_t i = 0; i < viewMsg.size(); i++)
	{
		viewMsg[i]->copy_to_buf(&buf[ptr]);
		ptr += viewMsg[i]->get_size();
	}

	assert(numPreMsgs == preMsg.size());
	for (uint64_t i = 0; i < numPreMsgs; i++)
	{
		preMsg[i]->copy_to_buf(&buf[ptr]);
		ptr += preMsg[i]->get_size();
	}

	assert(ptr == get_size());
}

void NewViewMsg::release()
{
	BatchRequests *pmsg;
	uint64_t i = 0;
	for (; i < preMsg.size();)
	{
		pmsg = preMsg[i];
		preMsg.erase(preMsg.begin() + i);
		Message::release_message(pmsg);
	}

	ViewChangeMsg *vmsg;
	for (i = 0; i < viewMsg.size();)
	{
		vmsg = viewMsg[i];
		viewMsg.erase(viewMsg.begin() + i);
		Message::release_message(vmsg);
	}
	viewMsg.clear();
}

string NewViewMsg::toString()
{
	string message;
	message += this->view;
	message += '_';

	for (uint64_t i = 0; i < viewMsg.size(); i++)
	{
		message += viewMsg[i]->toString();
		message += '_';
	}

	for (uint64_t i = 0; i < preMsg.size(); i++)
	{
		message += preMsg[i]->getString(this->return_node_id);
		message += '_';
	}

	return message;
}

void NewViewMsg::sign(uint64_t dest)
{
#if USE_CRYPTO
	string message = this->toString();

	signingNodeNode(message, this->signature, this->pubKey, dest);
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

bool NewViewMsg::validate(uint64_t thd_id)
{
	string message = this->toString();

#if USE_CRYPTO
	//verify signature of message
	if (!validateNodeNode(message, this->pubKey, this->signature, this->return_node_id))
	{
		assert(0);
		return false;
	}

#endif
	return true;
}

/*******************************/

// Entities for handling BatchRequests message during a view change.
vector<BatchRequests *> breqStore;
std::mutex bstoreMTX;

// Stores a BatchRequests message.
void storeBatch(BatchRequests *breq)
{
	bstoreMTX.lock();
	breqStore.push_back(breq);
	bstoreMTX.unlock();
}

// Removes all the BatchRequests message till the specified range.
void removeBatch(uint64_t range)
{
	uint64_t i = 0;
	BatchRequests *bmsg;
	bstoreMTX.lock();
	for (; i < breqStore.size();)
	{
		bmsg = breqStore[i];
		if (bmsg->index[get_batch_size() - 1] < range)
		{
			breqStore.erase(breqStore.begin() + i);
			Message::release_message(bmsg);
		}
		else
		{
			// The first message with greater index, break!
			break;
		}
	}
	bstoreMTX.unlock();
}

// Entities for handling ViewChange message.
vector<ViewChangeMsg *> view_change_msgs;

void storeVCMsg(ViewChangeMsg *vmsg)
{
	char *buf = create_msg_buffer(vmsg);
	Message *deepCMsg = deep_copy_msg(buf, vmsg);
	view_change_msgs.push_back((ViewChangeMsg *)deepCMsg);
	delete_msg_buffer(buf);
}

void clearAllVCMsg()
{
	ViewChangeMsg *vmsg;
	uint64_t i = 0;
	for (; i < view_change_msgs.size();)
	{
		vmsg = view_change_msgs[i];
		view_change_msgs.erase(view_change_msgs.begin() + i);
		Message::release_message(vmsg);
	}
	view_change_msgs.clear();
}

#endif // VIEW_CHANGES

/************************************/

#if CONSENSUS == HOTSTUFF

uint64_t HOTSTUFFPrepareMsg::get_size()
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
	size += highQC.get_size();

#if THRESHOLD_SIGNATURE
	size += sizeof(sig_share);
#endif

	return size;
}

void HOTSTUFFPrepareMsg::add_request_msg(uint idx, Message * msg){
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
void HOTSTUFFPrepareMsg::init(uint64_t instance_id)
{
	// // Only primary should create this message
	// #if !PVP
	// assert(get_view_primary(get_current_view(0)) == g_node_id);
	// #else
	// assert(get_view_primary(get_current_view(instance_id), instance_id) == g_node_id);
	// #endif
	this->view = get_current_view(instance_id);
	this->index.init(get_batch_size());
	this->requestMsg.resize(get_batch_size());
}

void HOTSTUFFPrepareMsg::copy_from_txn(TxnManager *txn)
{
	// Setting txn_id 2 less than the actual value.
	this->txn_id = txn->get_txn_id() - 2;
	this->batch_size = get_batch_size();
	// printf("[M1]\n");
	// fflush(stdout);
	// Storing the representative hash of the batch.
	this->hash = txn->hash;
	this->hashSize = txn->hashSize;
	// printf("[M2]\n");
	// fflush(stdout);
	// Use these lines for testing plain hash function.
	//string message = "anc_def";
	//this->hash.add(calculateHash(message));
}

#if BANKING_SMART_CONTRACT
void HOTSTUFFPrepareMsg::copy_from_txn(TxnManager *txn, BankingSmartContractMessage *clqry)
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
void HOTSTUFFPrepareMsg::copy_from_txn(TxnManager *txn, YCSBClientQueryMessage *clqry)
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

string HOTSTUFFPrepareMsg::getString(uint64_t sender){
	string message = std::to_string(sender);
	for (uint i = 0; i < get_batch_size(); i++)
	{
		message += std::to_string(index[i]);
		message += requestMsg[i]->getRequestString();
	}
	message += hash;

	return message;
}

void HOTSTUFFPrepareMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFPrepareMsg::copy_from_buf(char *buf)
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
	ptr = highQC.copy_from_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}


void HOTSTUFFPrepareMsg::copy_to_buf(char *buf)
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
	ptr = highQC.copy_to_buf(ptr, buf);
	
#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}


//makes sure message is valid, returns true for false
bool HOTSTUFFPrepareMsg::validate(uint64_t thd_id)
{

#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	try{
		unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
	} catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
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
	if (this->hash != calculateHash(batchStr) && this->rtype != NARWHAL_PAYLOAD_MSG)
	{
		assert(0);
		return false;
	}

	//verify threshold signature of highQC
#if THRESHOLD_SIGNATURE
#if !CHAINED
	assert(highQC.genesis || (highQC.ThresholdSignatureVerify(HOTSTUFF_PREP_MSG)));
#else
	assert(highQC.genesis || (highQC.ThresholdSignatureVerify(HOTSTUFF_NEW_VIEW_MSG)));
#endif
#endif

#if SHIFT_QC && !CHAINED
	if(highQC.genesis == false)
		assert(genericQC.ThresholdSignatureVerify(HOTSTUFF_NEW_VIEW_MSG));
#endif

	return true;
}

void HOTSTUFFPrepareMsg::release()
{
	index.release();
	for (uint64_t i = 0; i < requestMsg.size(); i++)
	{
		Message::release_message(requestMsg[i]);
	}
	requestMsg.clear();
}


uint64_t HOTSTUFFPrepareVoteMsg::get_size()
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
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFPrepareVoteMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFPrepareVoteMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFPrepareVoteMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	#if !PVP
	uint64_t instance_id = 0;
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	#endif
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
}

void HOTSTUFFPrepareVoteMsg::copy_from_buf(char *buf)
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

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFPrepareVoteMsg::copy_to_buf(char *buf)
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

#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFPrepareVoteMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
#endif

#endif

	return true;
}

uint64_t HOTSTUFFPreCommitMsg::get_size(){
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);
	size += PreparedQC.get_size();

#if THRESHOLD_SIGNATURE
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFPreCommitMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFPreCommitMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFPreCommitMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	#if !PVP
	uint64_t instance_id = 0;
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	#endif
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
	this->PreparedQC = txn->get_preparedQC();
}

void HOTSTUFFPreCommitMsg::copy_from_buf(char *buf)
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
	ptr = PreparedQC.copy_from_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFPreCommitMsg::copy_to_buf(char *buf)
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

	ptr = PreparedQC.copy_to_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFPreCommitMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
#endif

#endif

	return true;
}

uint64_t HOTSTUFFPreCommitVoteMsg::get_size()
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
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFPreCommitVoteMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFPreCommitVoteMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFPreCommitVoteMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	#if !PVP
	uint64_t instance_id = 0;
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	#endif
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
}

void HOTSTUFFPreCommitVoteMsg::copy_to_buf(char *buf)
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

#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFPreCommitVoteMsg::copy_from_buf(char *buf)
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

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFPreCommitVoteMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
#endif

#endif

	return true;
}

uint64_t HOTSTUFFCommitMsg::get_size(){
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);
	size += PreCommittedQC.get_size();

#if THRESHOLD_SIGNATURE
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFCommitMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFCommitMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFCommitMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	#if !PVP
	uint64_t instance_id = 0;
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	#endif
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
	this->PreCommittedQC = txn->get_precommittedQC();
}

void HOTSTUFFCommitMsg::copy_from_buf(char *buf)
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
	ptr = PreCommittedQC.copy_from_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFCommitMsg::copy_to_buf(char *buf)
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
	ptr = PreCommittedQC.copy_to_buf(ptr, buf);
	
#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFCommitMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
#endif

#endif

	return true;
}

uint64_t HOTSTUFFCommitVoteMsg::get_size()
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
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFCommitVoteMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFCommitVoteMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFCommitVoteMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	#if !PVP
	uint64_t instance_id = 0;
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	#endif
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
}

void HOTSTUFFCommitVoteMsg::copy_to_buf(char *buf)
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

#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFCommitVoteMsg::copy_from_buf(char *buf)
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

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFCommitVoteMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
#endif

#endif

	return true;
}

uint64_t HOTSTUFFDecideMsg::get_size(){
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);
	size += CommittedQC.get_size();

#if THRESHOLD_SIGNATURE
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFDecideMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFDecideMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}


void HOTSTUFFDecideMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	#if !PVP
	uint64_t instance_id = 0;
	#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	#endif
	this->view = get_current_view(instance_id);
	this->end_index = txn->get_txn_id();
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = txn->get_hashSize();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
	this->CommittedQC = txn->get_committedQC();
	assert(this->CommittedQC.type == COMMIT && !this->CommittedQC.batch_hash.empty());
}

void HOTSTUFFDecideMsg::copy_from_buf(char *buf)
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
	ptr = CommittedQC.copy_from_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFDecideMsg::copy_to_buf(char *buf)
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
	ptr = CommittedQC.copy_to_buf(ptr, buf);
	
#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif

	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFDecideMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
#endif

#endif

	return true;
}

uint64_t HOTSTUFFNewViewMsg::get_size(){
	uint64_t size = Message::mget_size();

	size += sizeof(view);
	size += sizeof(index);
	size += hash.length();
	size += sizeof(hashSize);
	size += sizeof(return_node);
	size += sizeof(end_index);
	size += sizeof(batch_size);
	size += PreparedQC.get_size();
#if THRESHOLD_SIGNATURE
	size += sizeof(sig_share);
#endif

	return size;
}

string HOTSTUFFNewViewMsg::toString()
{
	string signString = std::to_string(this->view);
	signString += '_' + std::to_string(this->index) + '_' +
				  this->hash + '_' + std::to_string(this->return_node) +
				  '_' + to_string(this->end_index);

	return signString;
}

void HOTSTUFFNewViewMsg::sign(uint64_t dest_node)
{
#if USE_CRYPTO
	#if THRESHOLD_SIGNATURE && ENABLE_ENCRYPT
		unsigned char message[32];
		memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
		assert(secp256k1_ecdsa_sign(ctx, &sig_share, message, private_key, NULL, NULL));
	#endif
#else
	this->signature = "0";
#endif
	this->sigSize = this->signature.size();
	this->keySize = this->pubKey.size();
}

void HOTSTUFFNewViewMsg::copy_from_txn(TxnManager *txn){
	Message::mcopy_from_txn(txn);
	this->txn_id = txn->get_txn_id();
#if !PVP
	this->view = get_current_view(0);
#else
	uint64_t instance_id = this->txn_id / get_batch_size() % get_totInstances();
	this->view = get_current_view(instance_id);
#endif
	this->end_index = this->txn_id;
	this->index = this->end_index + 1 - get_batch_size();
	this->hash = txn->get_hash();
	this->hashSize = this->hash.size();
	this->return_node = g_node_id;
	this->batch_size = get_batch_size();
#if !PVP
	this->PreparedQC = get_g_preparedQC();
#else
	this->PreparedQC = get_g_preparedQC(instance_id);
#endif
}

void HOTSTUFFNewViewMsg::copy_from_buf(char *buf)
{
	Message::mcopy_from_buf(buf);
	fflush(stdout);
	uint64_t ptr = Message::mget_size();
	fflush(stdout);
	COPY_VAL(view, buf, ptr);
	COPY_VAL(index, buf, ptr);
	COPY_VAL(hashSize, buf, ptr);

	ptr = buf_to_string(buf, ptr, hash, hashSize);

	COPY_VAL(return_node, buf, ptr);
	COPY_VAL(end_index, buf, ptr);
	COPY_VAL(batch_size, buf, ptr);

	ptr = PreparedQC.copy_from_buf(ptr, buf);

#if THRESHOLD_SIGNATURE
	COPY_VAL(sig_share, buf, ptr);
#endif

	assert(ptr == get_size());
}

void HOTSTUFFNewViewMsg::copy_to_buf(char *buf)
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
	ptr = PreparedQC.copy_to_buf(ptr, buf);
	
#if THRESHOLD_SIGNATURE
	COPY_BUF(buf, sig_share, ptr);
#endif
	assert(ptr == get_size());
}

//makes sure message is valid, returns true or false;
bool HOTSTUFFNewViewMsg::validate()
{
#if USE_CRYPTO

#if THRESHOLD_SIGNATURE
	try{
	unsigned char message[32];
	memcpy(message, get_secp_hash(this->hash, this->rtype).c_str(), 32);
	assert(secp256k1_ecdsa_verify(ctx, &sig_share, message, &public_keys[return_node_id]));
	}catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
#endif

#endif
	return true;
}
#endif	//CONSENSUS == HOTSTUFF

/************************************/
