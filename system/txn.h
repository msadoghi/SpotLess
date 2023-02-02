#ifndef _TXN_H_
#define _TXN_H_

#include "global.h"
#include "semaphore.h"
#include "array.h"
#include "message.h"
#include "smart_contract.h"

class Workload;
class Thread;
class table_t;
class BaseQuery;
class TxnQEntry;
class YCSBQuery;

class Transaction
{
public:
    void init();
    void reset(uint64_t pool_id);
    void release(uint64_t pool_id);
    txnid_t txn_id;
    uint64_t batch_id;
    RC rc;
};

class TxnStats
{
public:
    void init();
    void clear_short();
    void reset();
    void abort_stats(uint64_t thd_id);
    void commit_stats(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id, uint64_t timespan_long, uint64_t timespan_short);
    uint64_t starttime;
    uint64_t restart_starttime;
    uint64_t wait_starttime;
    uint64_t write_cnt;
    uint64_t abort_cnt;
    double total_process_time;
    double process_time;
    double total_local_wait_time;
    double local_wait_time;
    double total_remote_wait_time; // time waiting for a remote response
    double remote_wait_time;
    double total_abort_time;     // time spent in aborted query land
    double total_msg_queue_time; // time spent on outgoing queue
    double msg_queue_time;
    double total_work_queue_time; // time spent on work queue
    double work_queue_time;
    uint64_t total_work_queue_cnt;
    uint64_t work_queue_cnt;

    // short stats
    double work_queue_time_short;
    double cc_block_time_short;
    double cc_time_short;
    double msg_queue_time_short;
    double process_time_short;
    double network_time_short;

    double lat_network_time_start;
    double lat_other_time_start;

    //PBFT Stats
    double time_start_pre_prepare;
    double time_start_prepare;
    #if CONSENSUS == HOTSTUFF
    double time_start_precommit;
    #endif
    double time_start_commit;
    double time_start_execute;
};

/*
   Execution of transactions
   Manipulates/manages Transaction (contains txn-specific data)
   Maintains BaseQuery (contains input args, info about query)
*/
class TxnManager
{
public:
    virtual ~TxnManager() {}
    virtual void init(uint64_t thd_id, Workload *h_wl);
    virtual void reset();
    void clear();
    void reset_query();
    void release(uint64_t pool_id);
    void release_all_messages(uint64_t txn_id);

    Thread *h_thd;
    Workload *h_wl;

    virtual RC run_txn() = 0;
    void register_thread(Thread *h_thd);
    uint64_t get_thd_id();
    Workload *get_wl();
    void set_txn_id(txnid_t txn_id);
    txnid_t get_txn_id();
    void set_query(BaseQuery *qry);
    BaseQuery *get_query();
    bool is_done();
    void commit_stats();

    uint64_t get_rsp_cnt() { return rsp_cnt; }
    uint64_t incr_rsp(int i);
    uint64_t decr_rsp(int i);

    RC commit();
    RC start_commit();

    bool aborted;
    uint64_t return_id;
    RC validate();

    uint64_t get_batch_id() { return txn->batch_id; }
    void set_batch_id(uint64_t batch_id) { txn->batch_id = batch_id; }

    Transaction *txn;
#if BANKING_SMART_CONTRACT
    SmartContract *smart_contract;
#else
    BaseQuery *query; // Client query.
#endif
    uint64_t client_startts; // Client timestamp for this transaction.
    uint64_t client_id;      // Id of client that sent this transaction.

    string hash;       // Hash of the client query.
    uint64_t hashSize; // Size of hash.
    string get_hash();
    void set_hash(string hsh);
    uint64_t get_hashSize();

    // We need to maintain one copy of the whole BatchRequests messages sent
    // by the primary. We only maintain in last request of the batch.
    BatchRequests *batchreq;
    void set_primarybatch(BatchRequests *breq);

    uint64_t get_abort_cnt() { return abort_cnt; }
    uint64_t abort_cnt;
    int received_response(RC rc);
    bool waiting_for_response();
    RC get_rc() { return txn->rc; }
    void set_rc(RC rc) { txn->rc = rc; }

    bool prepared = false;
    uint64_t cbatch;

    uint64_t prep_rsp_cnt;
    vector<uint64_t> info_prepare;

#if RING_BFT
    bool is_cross_shard = false;
    bool involved_shards[NODE_CNT / SHARD_SIZE]{false};
#endif
#if SHARPER
    bool is_cross_shard = false;
    bool involved_shards[NODE_CNT / SHARD_SIZE]{false};
    uint64_t prep_rsp_cnt_arr[NODE_CNT / SHARD_SIZE];
    uint64_t decr_prep_rsp_cnt(int shard);
    uint64_t get_prep_rsp_cnt(int shard);
    uint64_t commit_rsp_cnt_arr[NODE_CNT / SHARD_SIZE];
    uint64_t decr_commit_rsp_cnt(int shard);
    uint64_t get_commit_rsp_cnt(int shard);
#endif

#if PVP
    uint64_t instance_id;
#endif

#if CONSENSUS == HOTSTUFF
    uint64_t view;
    HOTSTUFFPrepareMsg *prepmsg;
    void set_primarybatch(HOTSTUFFPrepareMsg *prep);
#if SEPARATE
    HOTSTUFFProposalMsg *propmsg;
    void set_primarybatch(HOTSTUFFProposalMsg *prep);
    bool generic_received;
#endif
    QuorumCertificate preparedQC;
    QuorumCertificate precommittedQC;
    QuorumCertificate committedQC;
#if CHAINED
    QuorumCertificate genericQC;
#endif
    QuorumCertificate highQC;
#if MAC_VERSION
    secp256k1_ecdsa_signature psig_share;
#endif
    uint64_t prepare_vote_cnt;
    vector<uint64_t> vote_prepare;
    uint64_t precommit_vote_cnt;
    vector<uint64_t> vote_precommit;
    uint64_t commit_vote_cnt;
    vector<uint64_t> vote_commit;
    uint64_t new_view_vote_cnt;
    vector<uint64_t> vote_new_view;
    void setPreparedQC(HOTSTUFFPreCommitMsg *pcmsg);
    void setPreCommittedQC(HOTSTUFFCommitMsg *cmsg);
    void setCommittedQC(HOTSTUFFDecideMsg *dmsg);
    QuorumCertificate get_preparedQC();
    QuorumCertificate get_precommittedQC();
    QuorumCertificate get_committedQC();
    bool precommited = false;
    bool is_precommitted();
    void set_precommitted();
    bool new_viewed = false;
    bool is_new_viewed();
    void set_new_viewed();
    void send_hotstuff_precommit_msgs();
    void send_hotstuff_commit_msgs();
    void send_hotstuff_decide_msgs();
    void send_hotstuff_prepare_vote();
    void send_hotstuff_precommit_vote();
    void send_hotstuff_commit_vote();
#if SEPARATE
    void send_hotstuff_generic();
#endif
#if !STOP_NODE_SET
    bool send_hotstuff_newview();
#else
    bool send_hotstuff_newview(bool &failednode);
#endif
#endif

    map<uint64_t, set<uint64_t>> voters;

    uint64_t decr_prep_rsp_cnt();
    uint64_t get_prep_rsp_cnt();
    bool is_prepared();
    void set_prepared();

    void send_pbft_prep_msgs();

    uint64_t commit_rsp_cnt;
    bool committed_local = false;
    vector<uint64_t> info_commit;

    // We need to store all the complete Commit mssg in the last txn of batch.
    vector<PBFTCommitMessage *> commit_msgs;
    void add_commit_msg(PBFTCommitMessage *pcmsg);

    uint64_t decr_commit_rsp_cnt();
    uint64_t get_commit_rsp_cnt();
    bool is_committed();
    void set_committed();

    void send_pbft_commit_msgs();

#if MULTI_ON
    // An additional field called instance_id, 
    // which is used to determine which instance the transaction belongs to
    uint64_t instance_id;
#endif

    int chkpt_cnt;
    bool chkpt_flag = false;

    bool is_chkpt_ready();
    void set_chkpt_ready();
    uint64_t decr_chkpt_cnt();
    uint64_t get_chkpt_cnt();
    void send_checkpoint_msgs();

    TxnStats txn_stats;

    bool set_ready() { return ATOM_CAS(txn_ready, 0, 1); }
    bool unset_ready() { return ATOM_CAS(txn_ready, 1, 0); }
    bool is_ready() { return txn_ready == true; }
    volatile int txn_ready;

protected:
    int rsp_cnt;
    sem_t rsp_mutex;
};

#endif
