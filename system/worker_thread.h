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
    void init_txn_man(YCSBClientQueryMessage *msg, bool is_equi_victim = false);
#endif
#if EXECUTION_THREAD
    void send_execute_msg();
    RC process_execute_msg(Message *msg);
#endif

    void unset_ready_txn(TxnManager * tman);

#if CONSENSUS == HOTSTUFF
#if SEPARATE
    void set_txn_man_fields(PVPProposalMsg *prop, uint64_t bid, bool is_equi_victim = false);
#endif
    RC process_client_batch_hotstuff(Message *msg);
    void create_and_send_hotstuff_prepare(ClientQueryBatch *msg, uint64_t tid);
    void equivocate(ClientQueryBatch *msg, uint64_t tid);
    bool hotstuff_new_viewed(PVPSyncMsg* msg);
    RC process_hotstuff_execute(Message *msg);
    RC process_hotstuff_new_view(Message *msg);
    void advance_view();
    void skip_view(PVPGenericMsg* gene);
    void skip_view(PVPSyncMsg* nvmsg, uint64_t txn_id);
#if SEPARATE
    RC process_hotstuff_generic_p(Message *msg);
    RC process_hotstuff_proposal(Message *msg);
#endif
    RC process_hotstuff_generic(Message *msg);
    void update_lockQC(const QuorumCertificate& QC, uint64_t view, uint64_t txnid, uint64_t instance_id);
    void send_execute_msg_hotstuff(uint64_t instance_id);
    void send_execute_msg_hotstuff(TxnManager *t_man, uint64_t instance_id);
#endif

#if EQUIV_TEST
    RC process_equivocate_generic(Message *msg);
#endif

#if TIMER_MANAGER
    void send_failed_new_view(uint64_t instance_id, uint64_t view);
    void process_failed_new_view(PVPSyncMsg *msg);
    void advance_failed_view(uint64_t instance_id, uint64_t view);
#endif

    RC process_pvp_ask(Message *msg);
    RC process_pvp_ask_response(Message *msg);
    RC send_ask_response(uint64_t dest_node, uint64_t start_id, uint64_t view);

private:
    uint64_t _thd_txn_id;
    ts_t _curr_ts;
    TxnManager *txn_man;
};

#endif
