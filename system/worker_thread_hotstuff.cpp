#include "global.h"
#include "message.h"
#include "thread.h"
#include "worker_thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "ycsb_query.h"
#include "math.h"
#include "msg_thread.h"
#include "msg_queue.h"
#include "work_queue.h"
#include "message.h"
#include "timer.h"
#include "chain.h"

#if CONSENSUS == HOTSTUFF
/**
 * This function is used by the primary replicas to create and set 
 * transaction managers for each transaction part of the ClientQueryBatch message sent 
 * by the client. Further, to ensure integrity a hash of the complete batch is 
 * generated, which is also used in future communication.
 *
 * @param msg Batch of transactions as a ClientQueryBatch message.
 * @param tid Identifier for the first transaction of the batch.
 */
void WorkerThread::create_and_send_hotstuff_prepare(ClientQueryBatch *msg, uint64_t tid){
#if !CHAINED
    Message *bmsg = Message::create_message(HOTSTUFF_PREP_MSG);
    HOTSTUFFPrepareMsg *prep = (HOTSTUFFPrepareMsg *)bmsg;
#elif SEPARATE
    Message *bmsg = Message::create_message(HOTSTUFF_PROPOSAL_MSG);
    HOTSTUFFProposalMsg *prep = (HOTSTUFFProposalMsg *)bmsg;
#else
    Message *bmsg = Message::create_message(HOTSTUFF_GENERIC_MSG);
    HOTSTUFFGenericMsg *prep = (HOTSTUFFGenericMsg *)bmsg;
#endif
    
#if !PVP
    prep->init(0);
#else
    uint64_t instance_id = msg->txn_id % get_totInstances();
    prep->init(instance_id);
#endif

    // Starting index for this batch of transactions.
    next_set = tid;

    // String of transactions in a batch to generate hash.
    string batchStr;
#if SEPARATE
    uint64_t view = get_last_sent_view(instance_id);
#endif
    // Allocate transaction manager for all the requests in batch.
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        uint64_t txn_id = get_next_txn_id() + i;

        //cout << "Txn: " << txn_id << " :: Thd: " << get_thd_id() << "\n";
        //fflush(stdout);
        txn_man = get_transaction_manager(txn_id, 0);

        // Unset this txn man so that no other thread can concurrently use.
        unset_ready_txn(txn_man);

        txn_man->register_thread(this);
        txn_man->return_id = msg->return_node;
        txn_man->view = view;
        txn_man->instance_id = instance_id;
        
        // Fields that need to updated according to the specific algorithm.
        algorithm_specific_update(msg, i);

        init_txn_man(msg->cqrySet[i]);

        // Append string representation of this txn.
        batchStr += msg->cqrySet[i]->getString();

        // Setting up data for HOTSTUFFPrepareMsg Message.
        prep->copy_from_txn(txn_man, msg->cqrySet[i]);

        // Reset this txn manager.
        bool ready = txn_man->set_ready();
        assert(ready);
    }

    // Now we need to unset the txn_man again for the last txn of batch.
    unset_ready_txn(txn_man);

    // Generating the hash representing the whole batch in last txn man.
    txn_man->set_hash(calculateHash(batchStr));
    assert(!txn_man->get_hash().empty());

#if CHAINED
    #if !PVP
        hash_QC_lock.lock();
        hash_to_txnid[txn_man->get_hash()] = txn_man->get_txn_id();
        hash_QC_lock.unlock();
    #else
        hash_QC_lock[instance_id].lock();
        hash_to_txnid[instance_id][txn_man->get_hash()] = txn_man->get_txn_id();
        hash_QC_lock[instance_id].unlock();
    #endif
#endif

    txn_man->hashSize = txn_man->hash.length();

    prep->copy_from_txn(txn_man);

// #if !SEPARATE
//     #if !PVP
//     prep->highQC = get_g_preparedQC();
//     #else
//     prep->highQC = get_g_preparedQC(instance_id);
//     #if SYNC_QC
//         prep->syncQC = get_g_syncQC(instance_id);
//     #endif
//     #endif 
// #endif

    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prep);

// #if !CHAINED
//     //push its own signature
//     prep->sign(g_node_id);
//     #if THRESHOLD_SIGNATURE
//         txn_man->preparedQC.signature_share_map[g_node_id] = prep->sig_share;
//     #endif
//     txn_man->vote_prepare.push_back(g_node_id);
// #endif

#if TIMER_ON && CHAINED
    #if !PVP
        add_timer(prep, txn_man->get_hash());
    #else
        add_timer(prep, txn_man->get_hash(), instance_id);
    #endif
#endif
    // Send the HOTSTUFFPrepareMsg message to all the other replicas.
    vector<uint64_t> dest = nodes_to_send(0, g_node_cnt);
    msg_queue.enqueue(get_thd_id(), prep, dest);
    dest.clear();
    #if PROPOSAL_THREAD
    // inc_incomplete_proposal_cnt(instance_id);
    #endif
// #if CHAINED && !SEPARATE
//     txn_man->send_hotstuff_newview();
//     advance_view();
// #endif

} 

/**
 * Processes an incoming client batch and sends a HOTSTUFFPrepareMsg message to al replicas.
 *
 * This function assumes that a client sends a batch of transactions and 
 * for each transaction in the batch, a separate transaction manager is created. 
 * Next, this batch is forwarded to all the replicas as a HOTSTUFFPrepareMsg Message, 
 * which corresponds to the Prepare first half stage in the PBFT protocol.
 *
 * @param msg Batch of Transactions of type CientQueryBatch from the client.
 * @return RC
 */
RC WorkerThread::process_client_batch_hotstuff(Message *msg)
{
    ClientQueryBatch *clbtch = (ClientQueryBatch *)msg;
    
    #if PROCESS_PRINT
    printf("ClientQueryBatch: %ld, THD: %ld :: CL: %ld :: RQ: %ld  %f\n", msg->txn_id, get_thd_id(), msg->return_node_id, clbtch->cqrySet[0]->requests[0]->key, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
    #endif

    // Authenticate the client signature.
    validate_msg(clbtch);
    // #if !PVP
    // if (g_node_id != get_view_primary(get_current_view(0)))
    // #else
    // uint64_t instance_id = clbtch->txn_id % get_totInstances();
    // if (g_node_id != get_view_primary(get_current_view(instance_id), instance_id))
    // #endif 
    // {
    //     //client_query_check(clbtch);
    //     #if !PVP
    //     cout << "returning...   " << get_view_primary(get_current_view(0)) << endl;
    //     #else
    //     cout << "returning...   " << get_view_primary(get_current_view(instance_id), instance_id) << endl;
    //     #endif
    //     return RCOK;
    // }

    // Initialize all transaction mangers and Send Prepare message.
    create_and_send_hotstuff_prepare(clbtch, clbtch->txn_id);
    return RCOK;
}

/**
 * Process incoming HOTSTUFFPrepareMsg message from the Primary.
 *
 * This function is used by the non-primary or backup replicas to process an incoming
 * Prepare message sent by the primary replica in HOTSTUFF. This processing would require 
 * sending messages of type HOTSTUFF_PREP_VOTE_MSG, which correspond to the Prepare phase of 
 * the HOTSTUFF protocol. Due to network delays, it is possible that a repica may have 
 * received a message of type HOTSTUFF_PRECOMMIT_MSG and HOTSTUFF_COMMIT_MSG, prior to 
 * receiving this HOTSTUFF_PREP_MSG message.
 *
 * @param msg Batch of Transactions of type HOTSTUFF_PREP_MSG from the primary.
 * @return RC
 */
RC WorkerThread::process_hotstuff_prepare(Message *msg)
{
    //uint64_t cntime = get_sys_clock();
    HOTSTUFFPrepareMsg* prep = (HOTSTUFFPrepareMsg *)msg;
#if PROCESS_PRINT
    printf("HOTSTUFF_PREP_MSG: TID:%ld : VIEW: %ld : THD: %ld  FROM: %ld   %lf\n",prep->txn_id, prep->view, get_thd_id(), prep->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif
    
#if !PVP
    if(get_view_primary(get_current_view(get_thd_id())) != prep->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", prep->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = prep->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != prep->return_node_id){
        printf("[A]Incorrect Leader %lu in this view %lu in instance %lu\n", prep->return_node_id, get_current_view(instance_id), instance_id);
#endif
        return RCOK;
    }

    // Check if the message is valid.
    #if ENABLE_ENCRYPT
    validate_msg(prep);
    #endif

#if !PVP
    if(!SafeNode(prep->highQC)){
        cout << "UnSafe Node" << endl;
#else
    if(!SafeNode(prep->highQC, instance_id, prep->txn_id / get_batch_size() / get_totInstances())){
        cout << "UnSafe Node in Instance " << instance_id << endl;
#endif
        return RCOK;
    }

    // Allocate transaction managers for all the transactions in the batch.
    set_txn_man_fields(prep, 0);
    // assert(txn_man->preparedQC.common_signature == NULL);
#if TIMER_ON
    // The timer for this client batch stores the hash of last request.
    #if !PVP
    server_timer->waiting_prepare = false;
    add_timer(prep, txn_man->get_hash());
    #else
    server_timer[instance_id]->waiting_prepare = false;
    add_timer(prep, txn_man->get_hash(), instance_id);
    #endif
#endif
    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prep);
    // Send HOTSTUFFPrepareVote messages.
    txn_man->send_hotstuff_prepare_vote();

    // Only when HOTSTUFFPrepareMsg message comes after some PreCommit message.
#if THRESHOLD_SIGNATURE
    if(txn_man->get_hash() == txn_man->preparedQC.batch_hash){
#if TS_SIMULATOR
        if(txn_man->preparedQC.signature_share_map.size() >= 1)
#else
        if(txn_man->preparedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
        {
            cout << "[A]" << endl;
            if(txn_man->preparedQC.ThresholdSignatureVerify(HOTSTUFF_PREP_MSG))
                txn_man->set_prepared();
        }
    }
#endif
    // If a PreCommit message has already arrived.
    if (txn_man->is_prepared())
    {
        uint64_t txnid = prep->txn_id + 2;
        // update the preparedQC in the chain
        #if !PVP
        set_g_preparedQC(txn_man->preparedQC);
        hash_QC_lock.lock();
        hash_to_txnid.insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, txnid));
        hash_QC_lock.unlock();
        #else
        hash_QC_lock[instance_id].lock();
        hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, txnid));
        txnid_to_hash[instance_id].insert(make_pair<uint64_t&,string&>(txnid, txn_man->preparedQC.batch_hash));
        hash_QC_lock[instance_id].unlock();
        set_g_preparedQC(txn_man->preparedQC, instance_id, txnid);
        #endif

        // Check if any Commit messages arrived before this Prepare message.
#if THRESHOLD_SIGNATURE
#if TS_SIMULATOR
        if(txn_man->precommittedQC.signature_share_map.size() >= 1)
#else
        if(txn_man->precommittedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
        {
            cout << "[B]" << endl;
            if(txn_man->precommittedQC.ThresholdSignatureVerify(HOTSTUFF_PRECOMMIT_MSG))
                txn_man->set_precommitted();
        }
#endif
        else{
            // Send PreCommit_Vote message.
            txn_man->send_hotstuff_precommit_vote();
        }

        // If a Commit message has already arrived.
        if (txn_man->is_precommitted())
        {
            // update the lockedQC in the chain
            #if !PVP
                set_g_lockedQC(txn_man->precommittedQC);
            #else
                set_g_lockedQC(txn_man->precommittedQC, instance_id, txnid);
            #endif

#if THRESHOLD_SIGNATURE
#if TS_SIMULATOR
            if(txn_man->committedQC.signature_share_map.size() >= 1)
#else
            if(txn_man->committedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
            {
                cout << "[C]" << endl;
                if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
                    txn_man->set_committed();
            }
#endif
            else{
                txn_man->send_hotstuff_commit_vote();
            }

            // If a Decide message has already arrived.
            if(txn_man->is_committed()){
                #if TIMER_ON
                    // End the timer for this client batch.
                    #if !PVP
                    remove_timer(txn_man->hash);
                    #else
                    remove_timer(txn_man->hash, instance_id);
                    #endif
                #endif

            txn_man->send_hotstuff_newview();
            if(txn_man->is_new_viewed())
                    advance_view();

#if !PVP
            send_execute_msg_hotstuff();
#else
            send_execute_msg_hotstuff(instance_id);
#endif
            }
        }
    }
    // Release this txn_man for other threads to use.
    bool ready = txn_man->set_ready();
    assert(ready);
    // UnSetting the ready for the txn id representing this batch.
    txn_man = get_transaction_manager(msg->txn_id, 0);
    unset_ready_txn(txn_man);

    return RCOK;
}

/**
 * Processes incoming PrepareVote message.
 *
 * This functions precessing incoming messages of type HOTSTUFF_PREP_VOTE_MSG. The primary creates 
 * and sends a HOTSTUFF_PRECOMMIT_MSG to the primary after receiving 2f+1 PrepareVote msgs.
 *
 * @param msg PrepareVote message of type HOTSTUFF_PREP_VOTE_MSG from a replica.
 * @return RC
 */

RC WorkerThread::process_hotstuff_prepare_vote(Message *msg)
{
    #if PROCESS_PRINT
    printf("HOTSTUFF_PREP_VOTE_MSG: TID: %ld FROM: %ld at %lf\n", msg->txn_id, msg->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
    #endif
    // Check if the incoming message is valid.
    HOTSTUFFPrepareVoteMsg *pvmsg = (HOTSTUFFPrepareVoteMsg *)msg;
    #if ENABLE_ENCRYPT
    validate_msg(pvmsg);
    #endif

    // If 2f+1 vote msgs have arrived 
    if(hotstuff_prepared(pvmsg)){
        txn_man->send_hotstuff_precommit_msgs();
    }
    return RCOK;
}


/**
 * Processes incoming PreCommit message.
 *
 * This functions precessing incoming messages of type HOTSTUFF_PRECOMMIT_MSG. A replica creates 
 * and sends a HOTSTUFF_PRECOMMIT_VOTE_MSG to the primary after receiving a PreCommit msg.
 *
 * @param msg PreCommit message of type HOTSTUFF_PRECOMMIT_MSG from a replica.
 * @return RC
 */
RC WorkerThread::process_hotstuff_precommit(Message *msg)
{
    #if PROCESS_PRINT
    cout << "HOTSTUFF_PRECOMMIT_MSG: TID: " << msg->txn_id << " FROM: " << msg->return_node_id << "   "<< simulation->seconds_from_start(get_sys_clock())<< endl;
    fflush(stdout);
    #endif

    // Check if the incoming message is valid.
    HOTSTUFFPreCommitMsg *pcmsg = (HOTSTUFFPreCommitMsg *)msg;

#if !PVP
    if(get_view_primary(get_current_view(get_thd_id())) != pcmsg->return_node_id){
        printf("Incorrect Leader %lu in view %lu\n", pcmsg->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = pcmsg->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != pcmsg->return_node_id){
        printf("[B]Incorrect Leader %lu in view %lu in instance %lu\n", pcmsg->return_node_id, get_current_view(instance_id), instance_id);
#endif
        return RCOK;
    }

    #if ENABLE_ENCRYPT
    validate_msg(pcmsg);
    #endif

    txn_man->setPreparedQC(pcmsg);

    if(!txn_man->get_hash().empty()){
        if(!txn_man->is_prepared()){
            if(checkMsg(msg)){
#if THRESHOLD_SIGNATURE
                if(txn_man->preparedQC.ThresholdSignatureVerify(HOTSTUFF_PREP_MSG))
#endif
                    txn_man->set_prepared();
            }
        }
    }

    if(txn_man->is_prepared()){
        // Store the preparedQC
        #if !PVP
        set_g_preparedQC(txn_man->preparedQC);
        hash_QC_lock.lock();
        hash_to_txnid.insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, pcmsg->txn_id));
        hash_QC_lock.unlock();
        #else        
        hash_QC_lock[instance_id].lock();
        txnid_to_hash[instance_id].insert(make_pair<uint64_t&,string&>(pcmsg->txn_id, txn_man->preparedQC.batch_hash));
        hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, pcmsg->txn_id));
        hash_QC_lock[instance_id].unlock();
        set_g_preparedQC(txn_man->preparedQC, instance_id, pcmsg->txn_id);
        #endif
        
        // Send PreCommit Vote message.
        txn_man->send_hotstuff_precommit_vote();

        // Check if any Commit messages arrived before this PreCommit message.
#if THRESHOLD_SIGNATURE
        if(txn_man->get_hash() == txn_man->precommittedQC.batch_hash){
#if TS_SIMULATOR
            if(txn_man->precommittedQC.signature_share_map.size() >= 1)
#else
            if(txn_man->precommittedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
            {
                cout << "[D]" << endl;
                if( txn_man->precommittedQC.ThresholdSignatureVerify(HOTSTUFF_PRECOMMIT_MSG))
                    txn_man->set_precommitted();
            }
        }
#endif

        // If a Commit message has already arrived.
        if (txn_man->is_precommitted())
        {
            // update the lockedQC in the chain
            #if !PVP
            set_g_lockedQC(txn_man->precommittedQC);
            #else
            set_g_lockedQC(txn_man->precommittedQC, instance_id, pcmsg->txn_id);
            #endif

            // Proceed to executing this batch of transactions.
            txn_man->send_hotstuff_commit_vote();

            // Check if any Decide messages arrived before this PreCommit message.
#if THRESHOLD_SIGNATURE
#if TS_SIMULATOR
            if(txn_man->committedQC.signature_share_map.size() >= 1)
#else
            if(txn_man->committedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
            {
                cout << "[E]" << endl;
                if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
                    txn_man->set_committed();
            }
#endif
            else{
                txn_man->send_hotstuff_commit_vote();
            }

            // If a Decide message has already arrived.
            if(txn_man->is_committed()){
                #if TIMER_ON
                    // End the timer for this client batch.
                    #if !PVP
                    remove_timer(txn_man->hash);
                    #else
                    remove_timer(txn_man->hash, instance_id);
                    #endif
                #endif
            
            txn_man->send_hotstuff_newview();
            if(txn_man->is_new_viewed())
                advance_view();

#if !PVP
            send_execute_msg_hotstuff();
#else
            send_execute_msg_hotstuff(instance_id);
#endif
            }
        }
    }
    return RCOK;
}

/**
 * Processes incoming PreCommitVote message.
 *
 * This functions precessing incoming messages of type HOTSTUFF_PRECOMMIT_VOTE_MSG. The primary creates 
 * and sends a HOTSTUFF_COMMIT_MSG to the primary after receiving 2f+1 PreCommitVote msgs.
 *
 * @param msg PreCommitVote message of type HOTSTUFF_PRECOMMIT_VOTE_MSG from a replica.
 * @return RC
 */

RC WorkerThread::process_hotstuff_precommit_vote(Message *msg)
{
    #if PROCESS_PRINT
    printf("HOTSTUFF_PRECOMMIT_VOTE_MSG: TID: %ld FROM: %ld at %lf\n", msg->txn_id, msg->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
    #endif
    // Check if the incoming message is valid.
    HOTSTUFFPreCommitVoteMsg *pcvmsg = (HOTSTUFFPreCommitVoteMsg *)msg;
    #if ENABLE_ENCRYPT
    validate_msg(pcvmsg);
    #endif

    // If the 2f+1 vote msgs have arrived
    if(hotstuff_precommitted(pcvmsg)){
#if PVP_RECOVERY
        fail_primary(5*BILLION, 0);
#endif
        txn_man->send_hotstuff_commit_msgs();
    }
    return RCOK;
}

/**
 * Processes incoming Commit message.
 *
 * This functions processing incoming messages of type HOTSTUFF_COMMIT_MSG. A replica creates 
 * and sends a HOTSTUFF_COMMIT_VOTE_MSG to the primary after receiving a Commit msg.
 *
 * @param msg Commit message of type HOTSTUFF_COMMIT_MSG from a replica.
 * @return RC
 */
RC WorkerThread::process_hotstuff_commit(Message *msg)
{
    #if PROCESS_PRINT
    cout << "HOTSTUFF_COMMIT_MSG: TID: " << msg->txn_id << " FROM: " << msg->return_node_id << "     " <<simulation->seconds_from_start(get_sys_clock())<< endl;
    fflush(stdout);
    #endif

    // Check if the incoming message is valid.
    HOTSTUFFCommitMsg *cmsg = (HOTSTUFFCommitMsg *)msg;

#if !PVP
    if(get_view_primary(get_current_view(get_thd_id())) != cmsg->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", cmsg->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = cmsg->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != cmsg->return_node_id){
        printf("[C]Incorrect Leader %lu in this view %lu in instance %lu\n", cmsg->return_node_id, get_current_view(instance_id), instance_id);
#endif
        return RCOK;
    }

    #if ENABLE_ENCRYPT
    validate_msg(cmsg);
    #endif

    txn_man->setPreCommittedQC(cmsg);

    if(!txn_man->get_hash().empty()){
        if(txn_man->is_prepared()){
            if(!txn_man->is_precommitted()){
                if(checkMsg(msg)){
#if THRESHOLD_SIGNATURE
                    if(txn_man->precommittedQC.ThresholdSignatureVerify(HOTSTUFF_PRECOMMIT_MSG))
#endif
                        txn_man->set_precommitted();
                }
            }
        }   
    }

    if(txn_man->is_precommitted()){
        // update the lockedQC in the chain
        #if !PVP
        set_g_lockedQC(txn_man->precommittedQC);
        #else
        set_g_lockedQC(txn_man->precommittedQC, instance_id, msg->txn_id);
        #endif

        // Send Commit Vote message.
        txn_man->send_hotstuff_commit_vote();

        // Check if any Decide message arrived before this Commit message.
#if THRESHOLD_SIGNATURE
        if(txn_man->get_hash() == txn_man->committedQC.batch_hash){
#if TS_SIMULATOR
            if(txn_man->committedQC.signature_share_map.size() >= 1)
#else
            if(txn_man->committedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
            {
                cout << "[F]" << endl;
                if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
                    txn_man->set_committed();
            }
        }    
#endif

        // If a Decide msg has arrived
        if(txn_man->is_committed()){
                #if TIMER_ON
                    // End the timer for this client batch.
                    #if !PVP
                    remove_timer(txn_man->hash);
                    #else
                    remove_timer(txn_man->hash, instance_id);
                    #endif
                #endif

            txn_man->send_hotstuff_newview();
            if(txn_man->is_new_viewed())
                advance_view();
#if !PVP
            send_execute_msg_hotstuff();
#else
            send_execute_msg_hotstuff(instance_id);
#endif
        }
    }
    return RCOK;
}

/**
 * Processes incoming CommitVote message.
 *
 * This functions precessing incoming messages of type HOTSTUFF_COMMIT_VOTE_MSG. The primary creates 
 * and sends a HOTSTUFF_DECIDE_MSG to the primary after receiving 2f+1 PreCommitVote msgs.
 *
 * @param msg CommitVote message of type HOTSTUFF_COMMIT_VOTE_MSG from a replica.
 * @return RC
 */

RC WorkerThread::process_hotstuff_commit_vote(Message *msg)
{
    #if PROCESS_PRINT
    printf("HOTSTUFF_COMMIT_VOTE_MSG: TID: %ld FROM: %ld at %lf\n", msg->txn_id, msg->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
    #endif
    // Check if the incoming message is valid.
    HOTSTUFFCommitVoteMsg *cvmsg = (HOTSTUFFCommitVoteMsg *)msg;
    #if ENABLE_ENCRYPT
    validate_msg(cvmsg);
    #endif

    // If the 2f+1 vote msgs have arrived
    if(hotstuff_committed(cvmsg)){
        // fail_primary(5*BILLION);
        txn_man->send_hotstuff_decide_msgs();
#if PVP
        uint64_t instance_id = msg->instance_id;
#endif

            txn_man->send_hotstuff_newview();
            advance_view();

#if !PVP
            send_execute_msg_hotstuff();
#else
            send_execute_msg_hotstuff(instance_id);
#endif
    }
    return RCOK;
}

/**
 * Processes incoming Decide message.
 *
 * This functions processing incoming messages of type HOTSTUFF_DECIDE_MSG. A replica creates 
 * and sends an execute msg to the itself after receiving a Decide msg.
 *
 * @param msg Deicde message of type HOTSTUFF_DECIDE_MSG from a replica.
 * @return RC
 */
RC WorkerThread::process_hotstuff_decide(Message *msg)
{
    #if PROCESS_PRINT
    cout << "HOTSTUFF_DECIDE_MSG: TID: " << msg->txn_id << " FROM: " << msg->return_node_id << "    " << simulation->seconds_from_start(get_sys_clock())<< endl;
    fflush(stdout);
    #endif

    // Check if the incoming message is valid.
    HOTSTUFFDecideMsg *dmsg = (HOTSTUFFDecideMsg *)msg;

#if !PVP
    if(get_view_primary(get_current_view(get_thd_id())) != dmsg->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", dmsg->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = dmsg->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != dmsg->return_node_id){
        printf("[D]Incorrect Leader %lu in this view %lu in instance %lu\n", dmsg->return_node_id, get_current_view(instance_id), instance_id);
#endif
        return RCOK;
    }

    #if ENABLE_ENCRYPT
    validate_msg(dmsg);
    #endif

    txn_man->setCommittedQC(dmsg);
    if(!txn_man->get_hash().empty()){
        if(txn_man->is_prepared() && txn_man->is_precommitted()){
            if(!txn_man->is_committed()){
                if(checkMsg(msg)){
#if THRESHOLD_SIGNATURE
                    if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
#endif
                        txn_man->set_committed();
                }
            }       
        }
    }
    if(txn_man->is_committed()){
            #if TIMER_ON
                // End the timer for this client batch.
                #if !PVP
                remove_timer(txn_man->hash);
                #else
                remove_timer(txn_man->hash, instance_id);
                #endif
            #endif

            txn_man->send_hotstuff_newview();
            if(txn_man->is_new_viewed())
                advance_view();
#if !PVP
            send_execute_msg_hotstuff();
#else
            send_execute_msg_hotstuff(instance_id);
#endif
    }
    return RCOK;
}

RC WorkerThread::process_hotstuff_execute(Message *msg){
    #if PROCESS_PRINT || SYNC_DEBUG
    // cout << "EXECUTE " << msg->txn_id << " THREAD: " << get_thd_id() << "\n";
    // fflush(stdout);
    #endif

    // cout << "EXECUTE " << msg->txn_id << " THREAD: " << get_thd_id() << "\n";
    // fflush(stdout);

    ExecuteMessage *emsg = (ExecuteMessage *)msg;

    if(!emsg->valid){
        cout << "INVALID" << endl;
        inc_next_index(get_batch_size());
        send_checkpoints(msg->txn_id+1);
        // Setting the next expected prepare message id.
        set_expectedExecuteCount(get_batch_size() + msg->txn_id);
        // cout << "MM" << endl;
        return RCOK;
    }

    // cout << "[E]";
    // fflush(stdout);
    uint64_t ctime = get_sys_clock();

    Message *rsp = Message::create_message(CL_RSP);
    ClientResponseMessage *crsp = (ClientResponseMessage *)rsp;
    crsp->init();

    // Execute transactions in a shot
    uint64_t i;
    for (i = emsg->index; i < emsg->end_index - 4; i++)
    {
        //cout << "i: " << i << " :: next index: " << g_next_index << "\n";
        //fflush(stdout);

        TxnManager *tman = get_transaction_manager(i, 0);

#if ENABLE_EXECUTE
        // Execute the transaction
        tman->run_txn();

        // Commit the results.
        tman->commit();
#endif
        INC_STATS(get_thd_id(), txn_cnt, 1);

        crsp->copy_from_txn(tman);
    }
    // Transactions (**95 - **98) of the batch.
    // We process these transactions separately, as we want to
    // ensure that their txn man are not held by some other thread.
    for (; i < emsg->end_index; i++)
    {
        TxnManager *tman = get_transaction_manager(i, 0);
        unset_ready_txn(tman);

#if ENABLE_EXECUTE
        // Execute the transaction
        tman->run_txn();

        // Commit the results.
        tman->commit();
#endif
        crsp->copy_from_txn(tman);

        INC_STATS(get_thd_id(), txn_cnt, 1);

        // Making this txn man available.
        bool ready = tman->set_ready();
        assert(ready);
    }

    // Last Transaction of the batch.
    txn_man = get_transaction_manager(i, 0);
    unset_ready_txn(txn_man);

#if ENABLE_EXECUTE
    // Execute the transaction
    txn_man->run_txn();

#if ENABLE_CHAIN
    // Add the block to the blockchain.
    BlockChain->add_block(txn_man);
#endif

    // Commit the results.
    txn_man->commit();
#endif

    // cout << "[F]";
    // fflush(stdout);

    INC_STATS(get_thd_id(), txn_cnt, 1);
    crsp->copy_from_txn(txn_man);
    
    inc_next_index(get_batch_size());

    vector<uint64_t> dest;
    dest.push_back(txn_man->client_id);
    msg_queue.enqueue(get_thd_id(), crsp, dest);
    dest.clear();

    
    INC_STATS(_thd_id, tput_msg, 1);
    INC_STATS(_thd_id, msg_cl_out, 1);

    // Check and Send checkpoint messages.
    send_checkpoints(msg->txn_id+1);
    // Setting the next expected prepare message id.
    set_expectedExecuteCount(get_batch_size() + msg->txn_id);
    // End the execute counter.
    INC_STATS(get_thd_id(), time_execute, get_sys_clock() - ctime);

    return RCOK;
}

RC WorkerThread::process_hotstuff_new_view(Message *msg){
    #if PROCESS_PRINT
    printf("HOTSTUFF_NEW_VIEW_MSG: TID: %lu THD: %lu FROM: %lu\n", msg->txn_id, get_thd_id(), msg->return_node_id);
    fflush(stdout);
    #endif
    // Check if the incoming message is valid.
    HOTSTUFFNewViewMsg *nvmsg = (HOTSTUFFNewViewMsg *)msg;
    #if ENABLE_ENCRYPT
    validate_msg(nvmsg);
    #endif

    // If the 2f+1 vote msgs have arrived
    if(hotstuff_new_viewed(nvmsg)){

#if PVP_RECOVERY && CHAINED
        #if !PVP
        fail_primary(10*BILLION, 0);
        #else
        fail_primary(10*BILLION, instance_id);
        #endif
#endif

#if CHAINED
#if SEPARATE
        if(!txn_man->get_hash().empty() && txn_man->generic_received)
#else
        if(!txn_man->get_hash().empty())
#endif
#else
        if(txn_man->is_committed())
#endif
        {
            advance_view();
        }
    }
    return RCOK;
}


#if CHAINED

#if !SEPARATE
/**
 * Process incoming HOTSTUFFGenericMsg message from the Primary.
 *
 * @param msg Batch of Transactions of type HOTSTUFF_PREP_MSG from the primary.
 * @return RC
 */
RC WorkerThread::process_hotstuff_generic(Message *msg)
{
    //uint64_t cntime = get_sys_clock();
    HOTSTUFFGenericMsg* gene = (HOTSTUFFGenericMsg *)msg;
#if PROCESS_PRINT
    printf("HOTSTUFF_GENERIC_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",gene->txn_id, gene->view, get_thd_id(), gene->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif

#if !PVP
    if(get_view_primary(get_current_view(get_thd_id())) != gene->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", gene->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = gene->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != gene->return_node_id){
        printf("[E]Incorrect Leader %lu in this view %lu in instance %lu\n", gene->return_node_id, get_current_view(instance_id), instance_id);
#endif  
        return RCOK;
    }

    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(gene);
#endif

#if !PVP
    if(!SafeNode(gene->highQC)){
        cout << "UnSafe Node" << endl;
#else
    if(!SafeNode(gene->highQC, instance_id, gene->txn_id / get_batch_size() / get_totInstances())){
        cout << "UnSafe Node in Instance " << instance_id << endl;
#endif
        return RCOK;
    }

    // Allocate transaction managers for all the transactions in the batch.
    set_txn_man_fields(gene, 0);

#if TIMER_ON
    // The timer for this client batch stores the hash of last request.
    #if !PVP
    server_timer->waiting_prepare = false;
    add_timer(gene, txn_man->get_hash());
    #else
    server_timer[instance_id]->waiting_prepare = false;
    add_timer(gene, txn_man->get_hash(), instance_id);
    #endif
#endif

    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(gene);

    uint64_t txnid = txn_man->get_txn_id();
    string hash = txn_man->get_hash();
#if !PVP
    hash_QC_lock.lock();
    hash_to_txnid.insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_to_QC[gene->highQC.batch_hash] = gene->highQC; 
    hash_QC_lock.unlock();
    update_lockQC(gene->highQC, txnid, gene->view);
#else
    hash_QC_lock[instance_id].lock();
    txnid_to_hash[instance_id].insert(make_pair<uint64_t&,string&>(txnid, hash));
    hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_to_QC[instance_id][gene->highQC.batch_hash] = gene->highQC;
    hash_QC_lock[instance_id].unlock();
    uint64_t view_number = gene->txn_id / get_batch_size() / get_totInstances();
#if SYNC_QC
    if(gene->highQC.viewNumber > get_g_preparedQC(instance_id).viewNumber)
#endif
        update_lockQC(gene->highQC, view_number, txnid - get_totInstances()*get_batch_size(), instance_id);
#endif
    
    txn_man->send_hotstuff_newview();
    if(txn_man->is_new_viewed())
        advance_view();

    // Release this txn_man for other threads to use.
    bool ready = txn_man->set_ready();
    assert(ready);
    // UnSetting the ready for the txn id representing this batch.
    txn_man = get_transaction_manager(msg->txn_id, 0);
    unset_ready_txn(txn_man);
    return RCOK;
}

#else   // SEPARATE
RC WorkerThread::process_hotstuff_proposal(Message *msg){
    //uint64_t cntime = get_sys_clock();
    HOTSTUFFProposalMsg* prop = (HOTSTUFFProposalMsg *)msg;
#if PROCESS_PRINT
    printf("HOTSTUFF_PROPOSAL_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",prop->txn_id, prop->view, get_thd_id(), prop->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif
    
    uint64_t instance_id = prop->instance_id;
    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(prop);
#endif
    // Allocate transaction managers for all the transactions in the batch.
    set_txn_man_fields(prop, 0);
    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prop);
    uint64_t txnid = txn_man->get_txn_id();
    string hash = txn_man->get_hash();

    hash_QC_lock[instance_id].lock();
    txnid_to_hash[instance_id].insert(make_pair<uint64_t&,string&>(txnid, hash));
    hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_QC_lock[instance_id].unlock();

    if(txn_man->is_new_viewed() && txn_man->generic_received){
        advance_view();
    }

    // Release this txn_man for other threads to use.
    bool ready = txn_man->set_ready();
    assert(ready);
    // UnSetting the ready for the txn id representing this batch.
    txn_man = get_transaction_manager(msg->txn_id, 0);
    unset_ready_txn(txn_man);
    return RCOK;
}

RC WorkerThread::process_hotstuff_generic(Message *msg){
    HOTSTUFFGenericMsg* gene = (HOTSTUFFGenericMsg *)msg;
#if PROCESS_PRINT
    printf("HOTSTUFF_GENERIC_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",gene->txn_id, gene->view, get_thd_id(), gene->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif

    uint64_t instance_id = gene->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != gene->return_node_id){
        printf("[E]Incorrect Leader %lu in this view %lu in instance %lu\n", gene->return_node_id, get_current_view(instance_id), instance_id);
        return RCOK;
    }

    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(gene);
#endif

    if(!SafeNode(gene->highQC, instance_id, gene->txn_id / get_batch_size() / get_totInstances())){
        cout << "UnSafe Node in Instance " << instance_id << endl;
        return RCOK;
    }

#if TIMER_MANAGER
    timer_manager[get_thd_id()].endTimer(instance_id);
#endif

    if(txn_man->get_hash().empty()){
        txn_man->view = gene->view;
        txn_man->instance_id = gene->instance_id;
    }

    txn_man->highQC = gene->highQC;
    txn_man->generic_received = true;

    uint64_t txnid = txn_man->get_txn_id();

    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][gene->highQC.batch_hash] = gene->highQC;
    hash_QC_lock[instance_id].unlock();
    // uint64_t view_number = gene->txn_id / get_batch_size() / get_totInstances();
    if(gene->highQC.viewNumber > get_g_preparedQC(instance_id).viewNumber)
        update_lockQC(gene->highQC, gene->view, txnid - get_totInstances() * get_batch_size(), instance_id);

    txn_man->send_hotstuff_newview();
    if(txn_man->is_new_viewed() && !txn_man->get_hash().empty())
        advance_view();

    return RCOK;
}

RC WorkerThread::process_hotstuff_generic_p(Message *msg){
    txn_man->send_hotstuff_generic();
    txn_man->generic_received = true;
    txn_man->send_hotstuff_newview();
    // uint64_t thd_id = txn_man->instance_id % get_multi_threads();
    // timer_manager[thd_id].setTimer(txn_man->instance_id);
    // msg->rtype = HOTSTUFF_GENERIC_MSG;
    return RCOK;
}

#endif

#endif

void WorkerThread::advance_view(bool update){
    uint64_t instance_id = txn_man->instance_id;
    uint64_t view = get_current_view(instance_id);
    uint64_t txn_id = txn_man->get_txn_id();
    view++;

#if CHAINED
    string hash = txn_man->get_hash();
    txn_man->genericQC.batch_hash = hash;
    assert(!hash.empty());
    txn_man->genericQC.type = PREPARE;
    txn_man->genericQC.viewNumber =  txn_man->highQC.genesis? 0:view - 1;
    txn_man->genericQC.parent_hash = txn_man->highQC.batch_hash;
    txn_man->genericQC.parent_view = txn_man->highQC.viewNumber;
    txn_man->genericQC.grand_hash = txn_man->highQC.parent_hash;
    txn_man->genericQC.grand_view = txn_man->highQC.parent_view;
    txn_man->genericQC.height = txn_man->highQC.height + 1;
    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][hash] = txn_man->genericQC;
    hash_QC_lock[instance_id].unlock();        
    update_lockQC(txn_man->genericQC, view, txn_id, instance_id);
#endif

    set_view(instance_id, view);
    // printf("jj[%lu] = %lu\n", instance_id, view);
    set_curr_new_viewed(txn_id, instance_id);
    
    if(g_node_id == get_view_primary(view, instance_id))
    {
        // printf("[AV1]%lu,%lu\n", instance_id, txn_id);
        #if SEPARATE
            sem_post(&new_txn_semaphore);
        #endif
    }else{
        #if TIMER_MANAGER
        timer_manager[get_thd_id()].setTimer(instance_id);
        #endif
    }
    if(g_node_id == get_view_primary(view + ROUNDS_IN_ADVANCE, instance_id)){ 
        //     printf("[AV2]%lu,%lu\n", instance_id, txn_id);
        #if SEPARATE
        #if PROPOSAL_THREAD
            sem_post(&proposal_semaphore);
        #else
            sem_post(&new_txn_semaphore);
        #endif
        #endif
    }

}

#if TIMER_MANAGER

void WorkerThread::send_failed_new_view(uint64_t instance_id, uint64_t view){
    printf("FAILED_NEW_VIEW %lu, %lu\n", instance_id, view);
    Message *msg = Message::create_message(HOTSTUFF_NEW_VIEW_MSG);
    HOTSTUFFNewViewMsg *nvmsg = (HOTSTUFFNewViewMsg*)msg;
    nvmsg->hashSize = nvmsg->hash.size();
    nvmsg->non_vote = true;
    nvmsg->txn_id = 0;
    nvmsg->view = view;
    nvmsg->instance_id = instance_id;
    nvmsg->highQC = get_g_preparedQC(instance_id);
    nvmsg->highQC.signature_share_map.clear();

    vector<uint64_t> dest = nodes_to_send(0, g_node_cnt);
    msg_queue.enqueue(get_thd_id(), nvmsg, dest);
    dest.clear();

    if(fault_manager.sufficient_voters(instance_id, view, g_node_id)){
        advance_failed_view(instance_id, view);
    }
}

void WorkerThread::process_failed_new_view(HOTSTUFFNewViewMsg *msg){
    if(fault_manager.sufficient_voters(msg->instance_id, msg->view, msg->return_node_id)){
        // printf("Perfect%lu %lu\n", msg->instance_id, msg->view);
        advance_failed_view(msg->instance_id, msg->view);
    }
}

void WorkerThread::advance_failed_view(uint64_t instance_id, uint64_t view){
    Message* msg = Message::create_message(EXECUTE_MSG);
    ExecuteMessage* emsg = (ExecuteMessage*)msg;
    emsg->txn_id = (get_totInstances() * view + instance_id + 1) * get_batch_size() - 2;
    emsg->valid = false;
    work_queue.enqueue(get_thd_id(), emsg, false);

    view++;

    set_view(instance_id, view);
    // set_curr_new_viewed(txn_id, instance_id);
    // printf("JJ[%lu] = %lu\n", instance_id, view);
    if(g_node_id == get_view_primary(view, instance_id))
    {
        // printf("[AV3]%lu,%lu\n", txn_id, instance_id);
        #if SEPARATE
            sem_post(&new_txn_semaphore);
        #endif
    }else{
        timer_manager[get_thd_id()].setTimer(instance_id);
    } 
    if(g_node_id == get_view_primary(view + ROUNDS_IN_ADVANCE, instance_id)){ 
        // printf("[AV4]%lu,%lu\n", instance_id, view);
        #if SEPARATE
        #if PROPOSAL_THREAD
            sem_post(&proposal_semaphore);
        #else
            sem_post(&new_txn_semaphore);
        #endif
        #endif
    }
}

#endif

#endif