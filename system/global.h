#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <unistd.h>
#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <typeinfo>
#include <list>
#include <map>
#include <unordered_map>
#include <set>
#include <queue>
#include <string>
#include <vector>
#include <sstream>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "./helper.h"
#include "pthread.h"
#include "../config.h"
#include "stats.h"
#include "pool.h"
#include "txn_table.h"
#include "sim_manager.h"
#include <mutex>

#include <unordered_map>
#include "xed25519.h"
#include "sha.h"
#include "database.h"
#include "hash_map.h"
#include "hash_set.h"

#include "semaphore.h"

#if THRESHOLD_SIGNATURE
#include <secp256k1.h>
#endif

using namespace std;

class mem_alloc;
class Stats;
class SimManager;
class Query_queue;
class Transport;
class Remote_query;
class TxnManPool;
class TxnPool;
class TxnTablePool;
class QryPool;
class TxnTable;
class QWorkQueue;
class MessageQueue;
class Client_query_queue;
class Client_txn;
class CommitCertificateMessage;
class ClientResponseMessage;
class RingBFTCommit;

#if CONSENSUS == HOTSTUFF

#endif  // CONSENSUS == HOTSTUFF

typedef uint32_t UInt32;
typedef int32_t SInt32;
typedef uint64_t UInt64;
typedef int64_t SInt64;

typedef uint64_t ts_t; // time stamp type

/******************************************/
// Global Data Structure
/******************************************/
extern mem_alloc mem_allocator;
extern Stats stats;
extern SimManager *simulation;
#if !FIX_MEM_LEAK
extern Query_queue query_queue;
extern TxnManPool txn_man_pool;
extern TxnTablePool txn_table_pool;
#endif
extern Client_query_queue client_query_queue;
extern Transport tport_man;
extern TxnPool txn_pool;
extern QryPool qry_pool;
extern TxnTable txn_table;
extern QWorkQueue work_queue;
extern MessageQueue msg_queue;
extern Client_txn client_man;

extern bool volatile warmup_done;
extern bool volatile enable_thread_mem_pool;
extern pthread_barrier_t warmup_bar;

/******************************************/
// Client Global Params
/******************************************/
extern UInt32 g_client_thread_cnt;
extern UInt32 g_client_rem_thread_cnt;
extern UInt32 g_client_send_thread_cnt;
extern UInt32 g_client_node_cnt;
extern UInt32 g_servers_per_client;
extern UInt32 g_clients_per_server;
extern UInt32 g_server_start_node;
extern uint64_t last_valid_txn;
uint64_t get_last_valid_txn();
void set_last_valid_txn(uint64_t txn_id);

/******************************************/
// Global Parameter
/******************************************/
extern volatile UInt64 g_row_id;
extern bool g_part_alloc;
extern bool g_mem_pad;
extern bool g_prt_lat_distr;
extern UInt32 g_node_id;
extern UInt32 g_node_cnt;
extern UInt32 g_part_cnt;
extern UInt32 g_virtual_part_cnt;
extern UInt32 g_core_cnt;
extern UInt32 g_total_node_cnt;
extern UInt32 g_total_thread_cnt;
extern UInt32 g_total_client_thread_cnt;
extern UInt32 g_this_thread_cnt;
extern UInt32 g_this_rem_thread_cnt;
extern UInt32 g_this_send_thread_cnt;
extern UInt32 g_this_total_thread_cnt;
extern UInt32 g_thread_cnt;
extern UInt32 g_execute_thd;
extern UInt32 g_sign_thd;
extern UInt32 g_send_thread_cnt;
extern UInt32 g_rem_thread_cnt;
extern UInt32 g_is_sharding;

extern UInt32 g_ts_alloc;
extern bool g_key_order;
extern bool g_ts_batch_alloc;
extern UInt32 g_ts_batch_num;
extern int32_t g_inflight_max;
extern uint64_t g_msg_size;

extern UInt32 g_max_txn_per_part;
extern int32_t g_load_per_server;

extern bool g_hw_migrate;
extern UInt32 g_network_delay;
extern UInt64 g_done_timer;
extern UInt64 g_batch_time_limit;
extern UInt64 g_seq_batch_time_limit;
extern UInt64 g_prog_timer;
extern UInt64 g_warmup_timer;
extern UInt64 g_msg_time_limit;

// YCSB
extern ts_t g_query_intvl;
extern UInt32 g_part_per_txn;
extern double g_perc_multi_part;
extern double g_txn_read_perc;
extern double g_txn_write_perc;
extern double g_tup_read_perc;
extern double g_tup_write_perc;
extern double g_zipf_theta;
extern double g_data_perc;
extern double g_access_perc;
extern UInt64 g_synth_table_size;
extern UInt32 g_req_per_query;
extern bool g_strict_ppt;
extern UInt32 g_field_per_tuple;
extern UInt32 g_init_parallelism;
extern double g_mpr;
extern double g_mpitem;

extern DataBase *db;

// Replication
extern UInt32 g_repl_type;
extern UInt32 g_repl_cnt;

enum RC
{
    RCOK = 0,
    Commit,
    FINISH,
    NONE
};

// Identifiers for Different Message types.
enum RemReqType
{
    INIT_DONE = 0,
    KEYEX,
    READY,
    CL_QRY, // Client Query
    RTXN,
    RTXN_CONT,
    RINIT,
    RDONE,
    CL_RSP, // Client Response
#if CLIENT_BATCH
    CL_BATCH, // Client Batch
#endif
    NO_MSG,

    EXECUTE_MSG, // Execute Notification
    BATCH_REQ,   // Pre-Prepare

#if VIEW_CHANGES
    VIEW_CHANGE,
    NEW_VIEW,
#endif
#if BANKING_SMART_CONTRACT
    BSC_MSG,
#endif
#if RING_BFT
    COMMIT_CERT_MSG,
    RING_PRE_PREPARE,
    RING_COMMIT,
#endif
#if SHARPER
    SUPER_PROPOSE,
#endif

    PBFT_PREP_MSG,   // Prepare
    PBFT_COMMIT_MSG, // Commit
    PBFT_CHKPT_MSG,   // Checkpoint and Garbage Collection

#if CONSENSUS == HOTSTUFF
    HOTSTUFF_PREP_MSG,
    HOTSTUFF_PREP_VOTE_MSG,
    HOTSTUFF_PRECOMMIT_MSG,
    HOTSTUFF_PRECOMMIT_VOTE_MSG,
    HOTSTUFF_COMMIT_MSG,
    HOTSTUFF_COMMIT_VOTE_MSG,
    HOTSTUFF_DECIDE_MSG,
    HOTSTUFF_NEW_VIEW_MSG,
    HOTSTUFF_GENERIC_MSG,
    NARWHAL_PAYLOAD_MSG
#endif

};

/* Thread */
typedef uint64_t txnid_t;

/* Txn */
typedef uint64_t txn_t;

typedef uint64_t idx_key_t;              // key id for index
typedef uint64_t (*func_ptr)(idx_key_t); // part_id func_ptr(index_key);

/* general concurrency control */
enum access_t
{
    RD,
    WR,
    XP,
    SCAN
};

#define GET_THREAD_ID(id) (id % g_thread_cnt)
#define GET_NODE_ID(id) (id % g_node_cnt)
#define GET_PART_ID(t, n) (n)
#define GET_PART_ID_FROM_IDX(idx) (g_node_id + idx * g_node_cnt)
#define GET_PART_ID_IDX(p) (p / g_node_cnt)
#define ISSERVER (g_node_id < g_node_cnt)
#define ISSERVERN(id) (id < g_node_cnt)
#define ISCLIENT (g_node_id >= g_node_cnt && g_node_id < g_node_cnt + g_client_node_cnt)
#define ISREPLICA (g_node_id >= g_node_cnt + g_client_node_cnt && g_node_id < g_node_cnt + g_client_node_cnt + g_repl_cnt * g_node_cnt)
#define ISREPLICAN(id) (id >= g_node_cnt + g_client_node_cnt && id < g_node_cnt + g_client_node_cnt + g_repl_cnt * g_node_cnt)
#define ISCLIENTN(id) (id >= g_node_cnt && id < g_node_cnt + g_client_node_cnt)

//@Suyash
#if DBTYPE != REPLICATED
#define IS_LOCAL(tid) (tid % g_node_cnt == g_node_id)
#else
#define IS_LOCAL(tid) true
#endif

#define IS_REMOTE(tid) (tid % g_node_cnt != g_node_id)
#define IS_LOCAL_KEY(key) (key % g_node_cnt == g_node_id)

#define MSG(str, args...)                                   \
    {                                                       \
        printf("[%s : %d] " str, __FILE__, __LINE__, args); \
    }

// principal index structure. The workload may decide to use a different
// index structure for specific purposes. (e.g. non-primary key access should use hash)
#if (INDEX_STRUCT == IDX_BTREE)
#define INDEX index_btree
#else // IDX_HASH
#define INDEX IndexHash
#endif

/************************************************/
// constants
/************************************************/
#ifndef UINT64_MAX
#define UINT64_MAX 18446744073709551615UL
#endif // UINT64_MAX

/******** Key storage for signing. ***********/
//ED25519 and RSA
extern string g_priv_key;                             //stores this node's private key
extern string g_public_key;                           //stores this node's public key
extern string g_pub_keys[NODE_CNT + CLIENT_NODE_CNT]; //stores public keys of other nodes
//CMAC
extern string cmacPrivateKeys[NODE_CNT + CLIENT_NODE_CNT];
extern string cmacOthersKeys[NODE_CNT + CLIENT_NODE_CNT];
// ED25519
extern CryptoPP::ed25519::Verifier verifier[NODE_CNT + CLIENT_NODE_CNT];
extern CryptoPP::ed25519::Signer signer;

// Receiving keys
extern uint64_t receivedKeys[NODE_CNT + CLIENT_NODE_CNT];

//Types for Keypair
typedef unsigned char byte;
#define copyStringToByte(byte, str)           \
    for (uint64_t i = 0; i < str.size(); i++) \
        byte[i] = str[i];
struct KeyPairHex
{
    std::string publicKey;
    std::string privateKey;
};

/*********************************************/

extern std::mutex keyMTX;
extern bool keyAvail;
extern uint64_t totKey;

extern uint64_t indexSize;
extern uint64_t g_min_invalid_nodes;

// Funtion to calculate hash of a string.
string calculateHash(string str);

// Entities for maintaining g_next_index.
extern uint64_t g_next_index; //index of the next txn to be executed
extern std::mutex gnextMTX;
void inc_next_index();
void inc_next_index(uint64_t val);
uint64_t curr_next_index();

#if CONSENSUS == HOTSTUFF

enum QCType{
    PREPARE,
    PRECOMMIT,
    COMMIT
};

class QuorumCertificate{
public:
    QCType type;
    bool genesis;
    uint64_t viewNumber;
    string batch_hash;
    string parent_hash;

#if THRESHOLD_SIGNATURE
    map<uint64_t, secp256k1_ecdsa_signature> signature_share_map;
#endif

    QuorumCertificate(uint _g_node_cnt = 0):type(PREPARE), viewNumber(0),  batch_hash(""), parent_hash(""){
        genesis = false;
        if(_g_node_cnt){
            genesis = true;
        }
    }

    ~QuorumCertificate(){
        signature_share_map.clear();
    }

    uint64_t get_size(){
        uint64_t size = sizeof(bool);
        size += sizeof(QCType); 
        size += sizeof(uint64_t);
        size += 2*sizeof(size_t);
        size += batch_hash.length();
        size += parent_hash.length();
#if THRESHOLD_SIGNATURE
        if(!genesis){
#if TS_SIMULATOR
            size_t map_size = signature_share_map.size() & 1;
#else
            size_t map_size = signature_share_map.size();
#endif
            size += sizeof(map_size);
            size += map_size * sizeof(uint64_t);
            size += map_size * sizeof(secp256k1_ecdsa_signature);
        }
#endif
        return size;
    }

    uint64_t copy_from_buf(uint64_t ptr, char *buf){
        COPY_VAL(type, buf, ptr);
    	COPY_VAL(genesis, buf, ptr);
        COPY_VAL(viewNumber, buf, ptr);
		size_t ssize;
        COPY_VAL(ssize, buf, ptr);

        batch_hash.resize(ssize);
        for(uint j=0; j<ssize; j++){
		    batch_hash[j] = buf[ptr+j];
	    }
        ptr += ssize;
        COPY_VAL(ssize, buf, ptr);
        parent_hash.resize(ssize);

        for(uint j=0; j<ssize; j++){
		    parent_hash[j] = buf[ptr+j];
	    }
        ptr += ssize;
#if THRESHOLD_SIGNATURE
        if(!genesis){
            size_t map_size;
            uint64_t node_id;
            secp256k1_ecdsa_signature sig_share;
            COPY_VAL(map_size, buf, ptr);
            for(size_t i=0 ; i<map_size; i++){
                COPY_VAL(node_id, buf, ptr);
                COPY_VAL(sig_share, buf, ptr);
                signature_share_map[i] = sig_share;
            }
        }
#endif
        return ptr;
    }

    uint64_t copy_to_buf(uint64_t ptr, char *buf){
        COPY_BUF(buf, type, ptr);
    	COPY_BUF(buf, genesis, ptr);
        COPY_BUF(buf, viewNumber, ptr);
        size_t ssize = batch_hash.length();
        COPY_BUF(buf, ssize, ptr);
        for(uint j=0; j<ssize; j++){
			buf[ptr+j] = batch_hash[j];
	    }
	    ptr += ssize;
        ssize = parent_hash.length();
        COPY_BUF(buf, ssize, ptr);
        for(uint j=0; j<ssize; j++){
			buf[ptr+j] = parent_hash[j];
	    }
	    ptr += ssize;

#if THRESHOLD_SIGNATURE
        if(!genesis){
#if TS_SIMULATOR
            size_t map_size = signature_share_map.size() & 1;
#else
            size_t map_size = signature_share_map.size();
#endif
            uint i = 0;
            COPY_BUF(buf, map_size, ptr);
            for(auto it = signature_share_map.begin(); it != signature_share_map.end() && i < map_size; ++it, ++i){
                COPY_BUF(buf, it->first, ptr);
                COPY_BUF(buf, it->second, ptr);
            }
        }       
#endif
        return ptr;
    }

#if THRESHOLD_SIGNATURE
    bool ThresholdSignatureVerify(RemReqType rtype);
#endif
};


// Entities for handling hotstuff_new_view_msgs
#if !PVP
extern uint32_t g_last_stable_new_viewed;
void set_curr_new_viewed(uint64_t txn_id);
uint64_t get_curr_new_viewed();
#else
extern uint32_t g_last_stable_new_viewed[MULTI_INSTANCES];
void set_curr_new_viewed(uint64_t txn_id, uint64_t instance_id);
uint64_t get_curr_new_viewed(uint64_t instance_id);
#endif
#endif

// Entities for handling checkpoints.
extern uint32_t g_last_stable_chkpt; //index of the last stable checkpoint
void set_curr_chkpt(uint64_t txn_id);
uint64_t get_curr_chkpt();

extern uint32_t g_txn_per_chkpt; // fixed interval of collecting checkpoints.
uint64_t txn_per_chkpt();

extern uint64_t lastDeletedTxnMan; // index of last deleted txn manager.
void inc_last_deleted_txn();
uint64_t get_last_deleted_txn();

extern uint g_checkpointing_thd;
extern uint64_t expectedExecuteCount;
extern uint64_t expectedCheckpoint;
uint64_t get_expectedExecuteCount();
void set_expectedExecuteCount(uint64_t val);

// Variable used by all threads during setup, to mark they are ready
extern std::mutex batchMTX;
extern uint commonVar;

// Variable used by Input thread at the primary to linearize batches.
extern uint64_t next_idx;
uint64_t get_and_inc_next_idx();
void set_next_idx(uint64_t val);

// Counters for input threads to access next socket (only used by replicas).
extern uint64_t sock_ctr[REM_THREAD_CNT];
uint64_t get_next_socket(uint64_t tid, uint64_t size);

// Global Utility functions:
vector<uint64_t> nodes_to_send(uint64_t beg, uint64_t end); // Destination for msgs.

// STORAGE OF CLIENT DATA
extern uint64_t ClientDataStore[SYNTH_TABLE_SIZE];

// Entities for MULTI_ON
#if MULTI_ON || PVP || NARWHAL
extern uint64_t totInstances;	// Number of parallel instances.
extern uint64_t multi_threads;  // Number of threads to manage these instances
uint64_t get_totInstances();
uint64_t get_multi_threads();

#if MULTI_ON
extern uint64_t current_primaries[MULTI_INSTANCES]; // List of primaries.
void set_primary(uint64_t nid, uint64_t idx);
uint64_t get_primary(uint64_t idx);

extern bool primaries[NODE_CNT];    // Whether a node is primary
void initialize_primaries();
bool isPrimary(uint64_t id);
#endif

#endif


#if CONSENSUS == HOTSTUFF
#if SEMA_TEST
// Entities for semaphore optimizations. The value of the semaphores means 
// the number of msgs in the corresponding queues of the worker_threads.
// Only worker_threads with msgs in their queues will be allocated with CPU resources.
extern sem_t narwhal_signal;
extern sem_t worker_queue_semaphore[THREAD_CNT];
// new_txn_semaphore is the number of instances that a replica is primary and has not sent a prepare msg.
extern sem_t new_txn_semaphore;
// execute_semaphore is whether the next msg to execute has been in the queue.
extern sem_t execute_semaphore;
// Entities for semaphore opyimizations on output_thread. The output_thread will be not allocated
// with CPU resources until there is a msg in its queue.
extern sem_t output_semaphore[SEND_THREAD_CNT];
// Semaphore indicating whether the setup is done
extern sem_t setup_done_barrier;

#if AUTO_POST
#if !PVP
extern bool auto_posted;
extern std::mutex auto_posted_lock;
extern void set_auto_posted(bool value);
extern bool is_auto_posted();
#else
extern bool auto_posted[MULTI_THREADS];
extern std::mutex auto_posted_lock[MULTI_THREADS];
extern void set_auto_posted(bool value, uint64_t instance_id);
extern bool is_auto_posted(uint64_t instance_id);
#endif
extern void* auto_post(void *ptr);
#endif

extern uint64_t init_msg_sent[SEND_THREAD_CNT];
extern void dec_init_msg_sent(uint64_t td_id);
extern uint64_t get_init_msg_sent(uint64_t td_id);
extern void init_init_msg_sent();
// A min heap storing txn_id of execute_msgs
extern std::priority_queue<uint64_t , vector<uint64_t>, greater<uint64_t> > execute_msg_heap;
extern std::mutex execute_msg_heap_lock;
void execute_msg_heap_push(uint64_t txn_id);
uint64_t execute_msg_heap_top();
void execute_msg_heap_pop();
#endif

extern uint64_t expectedInstance;

//Entities for client in HOTSTUFF and PVP.
//next_to_send is just the id of primary in the next round.
extern uint64_t next_to_send;
uint64_t get_next_to_send();
void inc_next_to_send();

//in_round is the value of batches that are sent but have not received enough responses.
extern uint64_t in_round[NODE_CNT];
uint64_t get_in_round(uint32_t node_id);
void inc_in_round(uint32_t node_id);
void dec_in_round(uint32_t node_id);

#if THRESHOLD_SIGNATURE

extern secp256k1_context *ctx;
extern unsigned char private_key[32];
extern secp256k1_pubkey public_key;
extern std::mutex public_key_lock;
extern map<uint64_t, secp256k1_pubkey> public_keys;
extern string get_secp_hash(string hash, RemReqType type);

#endif

#if !PVP
extern std::mutex hash_QC_lock;
extern unordered_map<string, QuorumCertificate> hash_to_QC;
extern unordered_map<string, uint64_t> hash_to_txnid;
extern unordered_map<string, uint64_t> hash_to_view;
#else
extern std::mutex hash_QC_lock[MULTI_INSTANCES];
extern vector<unordered_map<string, QuorumCertificate>> hash_to_QC;
extern vector<unordered_map<string, uint64_t>> hash_to_txnid;
extern vector<unordered_map<string, uint64_t>> hash_to_view;
#endif


#if !PVP
// if sent is true, a replica considers itself not as the next primary
// if sent is false, a replica considers itself as the next primary
extern bool sent;
extern QuorumCertificate g_preparedQC;
bool get_sent();
void set_sent(bool value);

// g_preparedQC is just the preparedQC in HOTSTUFF
const QuorumCertificate& get_g_preparedQC();
void set_g_preparedQC(const QuorumCertificate& QC);
uint64_t get_next_idx_hotstuff();

extern QuorumCertificate g_lockedQC;
const QuorumCertificate& get_g_lockedQC();
void set_g_lockedQC(const QuorumCertificate& QC);

#if SHIFT_QC && !CHAINED
extern QuorumCertificate g_genericQC;
const QuorumCertificate& get_g_genericQC();
void set_g_genericQC(const QuorumCertificate& QC);
#endif

extern bool SafeNode(const QuorumCertificate &highQC);

extern uint64_t get_view_primary(uint64_t view);

#else
extern bool sent[MULTI_INSTANCES];
extern QuorumCertificate g_preparedQC[MULTI_INSTANCES];
bool get_sent(uint64_t instance_id);
void set_sent(bool value, uint64_t instance_id);

const QuorumCertificate& get_g_preparedQC(uint64_t instance_id);
void set_g_preparedQC(const QuorumCertificate& QC, uint64_t instance_id);
uint64_t get_next_idx_hotstuff(uint64_t instance_id);

extern QuorumCertificate g_lockedQC[MULTI_INSTANCES];
const QuorumCertificate& get_g_lockedQC(uint64_t instance_id);
void set_g_lockedQC(const QuorumCertificate& QC, uint64_t instance_id);

#if SHIFT_QC && !CHAINED
extern QuorumCertificate g_genericQC[MULTI_INSTANCES];
const QuorumCertificate& get_g_genericQC(uint64_t instance_id);
void set_g_genericQC(const QuorumCertificate& QC, uint64_t instance_id);
#endif

extern bool SafeNode(const QuorumCertificate &highQC, uint64_t instance_id);
extern uint64_t get_view_primary(uint64_t view, uint64_t instance_id);
#endif

#endif

// Entities related to RBFT protocol.

// Entities pertaining to the current view.
uint64_t get_current_view(uint64_t thd_id);

//#if VIEW_CHANGES || MULTI_ON || PVP
// For updating view for input threads, batching threads, execute thread
// and checkpointing thread.
#if !PVP
extern std::mutex newViewMTX[THREAD_CNT + REM_THREAD_CNT + SEND_THREAD_CNT];
extern uint64_t view[THREAD_CNT + REM_THREAD_CNT + SEND_THREAD_CNT];
#else
extern std::mutex newViewMTX[MULTI_INSTANCES];
extern uint64_t view[MULTI_INSTANCES];
#endif
uint64_t get_view(uint64_t thd_id);
void set_view(uint64_t thd_id, uint64_t val);
//#endif

// Size of the batch.
extern uint64_t g_batch_size;
uint64_t get_batch_size();
extern uint64_t batchSet[2 * CLIENT_NODE_CNT * MAX_TXN_IN_FLIGHT];
#if SHARPER || RING_BFT
extern bool g_involved_shard[];
#endif
#if SHARPER
// extern uint32_t last_commited_txn;
extern uint32_t g_view[];
extern std::mutex viewMTX[];
uint64_t get_shard_number(uint64_t i = g_node_id);
uint64_t view_to_primary(uint64_t view, uint64_t node = g_node_id);
void set_client_view(uint64_t nview, int shard = 0);
uint64_t getget_client_view_view(int shard = 0);
uint64_t next_set_id(uint64_t prev);
int is_in_same_shard(uint64_t first_id, uint64_t second_id);
// bool is_local_request(uint64_t txn_id);
bool is_primary_node(uint64_t thd_id, uint64_t node = g_node_id);
extern UInt32 g_shard_size;
extern UInt32 g_shard_cnt;
extern UInt32 g_involved_shard_cnt;
extern SpinLockMap<string, int> digest_directory;
#elif RING_BFT
extern uint32_t g_view[];
extern std::mutex viewMTX[];
uint64_t get_shard_number(uint64_t i = g_node_id);
uint64_t view_to_primary(uint64_t view, uint64_t node = g_node_id);
void set_client_view(uint64_t nview, int shard = 0);
uint64_t get_client_view(int shard = 0);
uint64_t next_set_id(uint64_t prev);
int is_in_same_shard(uint64_t first_id, uint64_t second_id);
bool is_local_request(TxnManager *tman);
bool is_primary_node(uint64_t thd_id, uint64_t node = g_node_id);
bool is_sending_ccm(uint64_t node_id);
uint64_t sending_ccm_to(uint64_t node_id, uint64_t shard_id);
extern UInt32 g_shard_size;
extern UInt32 g_shard_cnt;
extern UInt32 g_involved_shard_cnt;
extern SpinLockMap<string, uint64_t> digest_directory;
extern SpinLockMap<string, CommitCertificateMessage *> ccm_directory;
extern SpinLockSet<string> rcm_checklist;
extern SpinLockSet<string> ccm_checklist;
#else
// This variable is mainly used by the client to know its current primary.
extern uint32_t g_view;
extern std::mutex viewMTX;
void set_client_view(uint64_t nview);
uint64_t get_client_view();
#endif

#if LOCAL_FAULT
extern uint64_t num_nodes_to_fail;
#endif

//Statistics global print variables -- only used in stats.cpp.
extern double idle_worker_times[THREAD_CNT];

// Statistics to print output_thread_idle_times.
extern double output_thd_idle_time[SEND_THREAD_CNT];
extern double input_thd_idle_time[REM_THREAD_CNT];

#if FIX_CL_INPUT_THREAD_BUG
extern std::mutex client_response_lock;
#endif
extern SpinLockMap<uint64_t, uint64_t> client_responses_count;
extern SpinLockMap<uint64_t, ClientResponseMessage *> client_responses_directory;

// Payload for messages.
#if PAYLOAD_ENABLE
extern uint64_t payload_size;
#endif

#if BANKING_SMART_CONTRACT
enum BSCType
{
    BSC_TRANSFER = 0,
    BSC_DEPOSIT = 1,
    BSC_WITHDRAW = 2,
};
#endif

#endif

