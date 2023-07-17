#ifndef _WORKERTHREAD_H_
#define _WORKERTHREAD_H_

#include "global.h"
#include "message.h"
#include "crypto.h"

class Workload;
class Message;

class WorkerThread : public Thread
{
public:
    ~WorkerThread();
    RC run();
    void setup();
    void send_key();
#if THRESHOLD_SIGNATURE && CONSENSUS == HOTSTUFF
    void send_public_key();
#endif
    RC process_key_exchange(Message *msg);

    void process(Message *msg);
    TxnManager *get_transaction_manager(uint64_t txn_id, uint64_t batch_id);
    RC init_phase();
    bool is_cc_new_timestamp();
    bool exception_msg_handling(Message *msg);

    uint64_t next_set;
    uint64_t get_next_txn_id();

    void release_txn_man(uint64_t txn_id, uint64_t batch_id);
    void algorithm_specific_update(Message *msg, uint64_t idx);

    void create_and_send_batchreq(ClientQueryBatch *msg, uint64_t tid);
    void set_txn_man_fields(BatchRequests *breq, uint64_t bid);

    bool validate_msg(Message *msg);
    bool checkMsg(Message *msg);
    RC process_client_batch(Message *msg);
    RC process_batch(Message *msg);
    void send_checkpoints(uint64_t txn_id);
    RC process_pbft_chkpt_msg(Message *msg);

#if BANKING_SMART_CONTRACT
    void init_txn_man(BankingSmartContractMessage *bscm);
#else
    void init_txn_man(YCSBClientQueryMessage *msg);
#endif
#if EXECUTION_THREAD
    void send_execute_msg();
    RC process_execute_msg(Message *msg);
#endif

#if VIEW_CHANGES
    void client_query_check(ClientQueryBatch *clbtch);
    void check_for_timeout();
    void store_batch_msg(BatchRequests *breq);
    RC process_view_change_msg(Message *msg);
    RC process_new_view_msg(Message *msg);
    void reset();
    void fail_primary(Message *msg, uint64_t time);
#endif

#if LOCAL_FAULT
    void fail_nonprimary();
#endif

    bool prepared(PBFTPrepMessage *msg);
    RC process_pbft_prep_msg(Message *msg);

    bool committed_local(PBFTCommitMessage *msg);
    RC process_pbft_commit_msg(Message *msg);
    void unset_ready_txn(TxnManager * tman);
#if SHARPER
    void create_and_send_batchreq_cross(ClientQueryBatch *msg, uint64_t tid);
    RC process_super_propose(Message *msg);
#elif RING_BFT
    RC process_commit_certificate(Message *msg);
    RC process_ringbft_preprepare(Message *msg);
    RC process_ringbft_commit(Message *msg);
    void create_and_send_pre_prepare(CommitCertificateMessage *msg, uint64_t tid);
#endif

    void read_txns(HOTSTUFFPrepareMsg *prep, uint64_t bid);
#if CONSENSUS == HOTSTUFF
    void set_txn_man_fields(HOTSTUFFPrepareMsg *prep, uint64_t bid);
    RC process_client_batch_hotstuff(Message *msg);
    void create_and_send_hotstuff_prepare(ClientQueryBatch *msg, uint64_t tid);
    void create_and_send_narwhal_payload(HOTSTUFFGenericMsg *msg);
    bool hotstuff_prepared(HOTSTUFFPrepareVoteMsg* msg);
    bool hotstuff_precommitted(HOTSTUFFPreCommitVoteMsg* msg);
    bool hotstuff_committed(HOTSTUFFCommitVoteMsg* msg);
    bool hotstuff_new_viewed(HOTSTUFFNewViewMsg* msg);
    RC process_hotstuff_prepare(Message *msg);
    RC process_hotstuff_precommit(Message *msg);
    RC process_hotstuff_commit(Message *msg);
    RC process_hotstuff_prepare_vote(Message *msg);
    RC process_hotstuff_precommit_vote(Message *msg);
    RC process_hotstuff_commit_vote(Message *msg);
    RC process_hotstuff_decide(Message *msg);
    RC process_hotstuff_execute(Message *msg);
    RC process_hotstuff_new_view(Message *msg);
    void advance_view(bool update = true);
    RC process_narwhal_payload(Message *msg);
#if CHAINED
    RC process_hotstuff_generic(Message *msg);
    
    #if !MUL
    void update_lockQC(const QuorumCertificate& QC, uint64_t view);
    #else
    void update_lockQC(const QuorumCertificate& QC, uint64_t view, uint64_t instance_id);
    #endif
#endif
    #if !MUL
        void send_execute_msg_hotstuff();
        void send_execute_msg_hotstuff(TxnManager *t_man);
    #else
        void send_execute_msg_hotstuff(uint64_t instance_id);
        void send_execute_msg_hotstuff(TxnManager *t_man, uint64_t instance_id);
    #endif
#endif

#if TESTING_ON
    void testcases(Message *msg);
#if TEST_CASE == ONLY_PRIMARY_NO_EXECUTE
    void test_no_execution(Message *msg);
#elif TEST_CASE == ONLY_PRIMARY_EXECUTE
    void test_only_primary_execution(Message *msg);
#elif TEST_CASE == ONLY_PRIMARY_BATCH_EXECUTE
    void test_only_primary_batch_execution(Message *msg);
#endif
#endif

private:
    uint64_t _thd_txn_id;
    ts_t _curr_ts;
    TxnManager *txn_man;
};

#endif
