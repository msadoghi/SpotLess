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
#else
    Message *bmsg = Message::create_message(HOTSTUFF_GENERIC_MSG);
    HOTSTUFFGenericMsg *prep = (HOTSTUFFGenericMsg *)bmsg;
#endif

#if !MUL
    prep->init(0);
#else
    uint64_t instance_id = msg->txn_id % get_totInstances();
    prep->init(instance_id);
#endif

    // Starting index for this batch of transactions.
    next_set = tid;

    // String of transactions in a batch to generate hash.
    string batchStr;

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
    #if !MUL
        hash_QC_lock.lock();
        hash_to_txnid[txn_man->get_hash()] = txn_man->get_txn_id();
        hash_to_view[txn_man->get_hash()] = get_current_view(0);
        hash_QC_lock.unlock();
    #else
        hash_QC_lock[instance_id].lock();
        hash_to_txnid[instance_id][txn_man->get_hash()] = txn_man->get_txn_id();
        hash_to_view[instance_id][txn_man->get_hash()] = get_current_view(instance_id);
        hash_QC_lock[instance_id].unlock();
    #endif
#endif

    txn_man->hashSize = txn_man->hash.length();

    prep->copy_from_txn(txn_man);

    #if !MUL
    prep->highQC = get_g_preparedQC();
    #else
    prep->highQC = get_g_preparedQC(instance_id);
    #endif 

    #if SHIFT_QC && !CHAINED
    if(prep->highQC.genesis == false){
        #if !MUL
        prep->genericQC = get_g_genericQC();
        #else
        prep->genericQC = get_g_genericQC(instance_id);
        #endif    
    }
    #endif

    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prep);

#if !CHAINED
    //push its own signature
    prep->sign(g_node_id);
    #if THRESHOLD_SIGNATURE
        txn_man->preparedQC.signature_share_map[g_node_id] = prep->sig_share;
    #endif
    txn_man->vote_prepare.push_back(g_node_id);
#endif

    create_and_send_narwhal_payload(prep);

    // Send the HOTSTUFFPrepareMsg message to all the other replicas.
    vector<uint64_t> dest = nodes_to_send(0, g_node_cnt);
    msg_queue.enqueue(get_thd_id(), prep, dest);
    dest.clear();

#if CHAINED
        txn_man->send_hotstuff_newview();
        advance_view();
#endif

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
    #if !MUL
    if (g_node_id != get_view_primary(get_current_view(0)))
    #else
    uint64_t instance_id = clbtch->txn_id % get_totInstances();
    if (g_node_id != get_view_primary(get_current_view(instance_id), instance_id))
    #endif 
    {
        //client_query_check(clbtch);
        #if !MUL
        cout << "returning...   " << get_view_primary(get_current_view(0)) << endl;
        #else
        cout << "returning...   " << get_view_primary(get_current_view(instance_id), instance_id) << endl;
        #endif
        return RCOK;
    }

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
    
#if !MUL
    if(get_view_primary(get_current_view(get_thd_id())) != prep->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", prep->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = prep->txn_id / get_batch_size() % get_totInstances();
    if(get_view_primary(get_current_view(instance_id), instance_id) != prep->return_node_id){
        printf("[A]Incorrect Leader %lu in this view %lu in instance %lu\n", prep->return_node_id, get_current_view(instance_id), instance_id);
#endif
        return RCOK;
    }

    // Check if the message is valid.
    #if ENABLE_ENCRYPT
    validate_msg(prep);
    #endif

#if !MUL
    if(!SafeNode(prep->highQC)){
        cout << "UnSafe Node" << endl;
#else
    if(!SafeNode(prep->highQC, instance_id)){
        cout << "UnSafe Node in Instance " << instance_id << endl;
#endif
        return RCOK;
    }

    // Allocate transaction managers for all the transactions in the batch.
    set_txn_man_fields(prep, 0);
    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prep);
    // Send HOTSTUFFPrepareVote messages.
    txn_man->send_hotstuff_prepare_vote();
    // End the counter for pre-prepare phase as prepare phase starts next.
    
    // Only when HOTSTUFFPrepareMsg message comes after some PreCommit message.
#if THRESHOLD_SIGNATURE
    if(txn_man->get_hash() == txn_man->preparedQC.batch_hash){
#if TS_SIMULATOR
        if(txn_man->preparedQC.signature_share_map.size() >= 1)
#else
        if(txn_man->preparedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
        {
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
        #if !MUL
        set_g_preparedQC(txn_man->preparedQC);
        hash_QC_lock.lock();
        hash_to_txnid.insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, txnid));
        hash_to_view.insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, prep->view));
        hash_QC_lock.unlock();
        #else
        set_g_preparedQC(txn_man->preparedQC, instance_id);
        hash_QC_lock[instance_id].lock();
        hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, txnid));
        hash_to_view[instance_id].insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, prep->view));
        hash_QC_lock[instance_id].unlock();
        #endif

        // Check if any Commit messages arrived before this Prepare message.
#if THRESHOLD_SIGNATURE
#if TS_SIMULATOR
        if(txn_man->precommittedQC.signature_share_map.size() >= 1)
#else
        if(txn_man->precommittedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
        {
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
            #if !MUL
                set_g_lockedQC(txn_man->precommittedQC);
            #else
                set_g_lockedQC(txn_man->precommittedQC, instance_id);
            #endif

#if THRESHOLD_SIGNATURE
#if TS_SIMULATOR
            if(txn_man->committedQC.signature_share_map.size() >= 1)
#else
            if(txn_man->committedQC.signature_share_map.size() >= 2*g_min_invalid_nodes+1)
#endif
            {
                if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
                    txn_man->set_committed();
            }
#endif
            else{
                txn_man->send_hotstuff_commit_vote();
            }

            // If a Decide message has already arrived.
            if(txn_man->is_committed()){

            if(!txn_man->send_hotstuff_newview()){
                if(txn_man->is_new_viewed()){
                    advance_view();
                }
            }else{
                advance_view();
            }

#if !MUL
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

#if !MUL
    if(get_view_primary(get_current_view(get_thd_id())) != pcmsg->return_node_id){
        printf("Incorrect Leader %lu in view %lu\n", pcmsg->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = pcmsg->txn_id / get_batch_size() % get_totInstances();
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
        #if !MUL
        set_g_preparedQC(txn_man->preparedQC);
        hash_QC_lock.lock();
        hash_to_txnid.insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, pcmsg->txn_id));
        hash_QC_lock.unlock();
        #else        
        set_g_preparedQC(txn_man->preparedQC, instance_id);
        hash_QC_lock[instance_id].lock();
        hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, pcmsg->txn_id));
        hash_QC_lock[instance_id].unlock();
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
                if( txn_man->precommittedQC.ThresholdSignatureVerify(HOTSTUFF_PRECOMMIT_MSG))
                    txn_man->set_precommitted();
            }
        }
#endif

        // If a Commit message has already arrived.
        if (txn_man->is_precommitted())
        {
            // update the lockedQC in the chain
            #if !MUL
            set_g_lockedQC(txn_man->precommittedQC);
            #else
            set_g_lockedQC(txn_man->precommittedQC, instance_id);
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
                if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
                    txn_man->set_committed();
            }
#endif
            else{
                txn_man->send_hotstuff_commit_vote();
            }

            // If a Decide message has already arrived.
            if(txn_man->is_committed()){

                if(!txn_man->send_hotstuff_newview()){
                    if(txn_man->is_new_viewed()){
                        advance_view();
                    }
                }else{
                    advance_view();
                }

#if !MUL
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

#if !MUL
    if(get_view_primary(get_current_view(get_thd_id())) != cmsg->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", cmsg->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = cmsg->txn_id / get_batch_size() % get_totInstances();
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
        #if !MUL
        set_g_lockedQC(txn_man->precommittedQC);
        #else
        set_g_lockedQC(txn_man->precommittedQC, instance_id);
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
                if(txn_man->committedQC.ThresholdSignatureVerify(HOTSTUFF_COMMIT_MSG))
                    txn_man->set_committed();
            }
        }    
#endif

        // If a Decide msg has arrived
        if(txn_man->is_committed()){

                if(!txn_man->send_hotstuff_newview()){
                    if(txn_man->is_new_viewed()){
                        advance_view();
                    }
                }else{
                    advance_view();
                }
#if !MUL
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
#if MUL
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
#endif
        
        txn_man->send_hotstuff_newview();
        advance_view();
#if !MUL
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

#if !MUL
    if(get_view_primary(get_current_view(get_thd_id())) != dmsg->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", dmsg->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = dmsg->txn_id / get_batch_size() % get_totInstances();
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
            if(!txn_man->send_hotstuff_newview()){
                if(txn_man->is_new_viewed()){
                    advance_view();
                }
            }else{
                advance_view();
            }

#if !MUL
            send_execute_msg_hotstuff();
#else
            send_execute_msg_hotstuff(instance_id);
#endif
    }
    return RCOK;
}

RC WorkerThread::process_hotstuff_execute(Message *msg){
    #if PROCESS_PRINT
    cout << "EXECUTE " << msg->txn_id << " THREAD: " << get_thd_id() << "\n";
    fflush(stdout);
    #endif
    uint64_t ctime = get_sys_clock();

    Message *rsp = Message::create_message(CL_RSP);
    ClientResponseMessage *crsp = (ClientResponseMessage *)rsp;
    crsp->init();

    ExecuteMessage *emsg = (ExecuteMessage *)msg;
// #if NARWHAL
    double tstart = simulation->seconds_from_start(get_sys_clock());
// #endif
    // Execute transactions in a shot
    uint64_t i;
    for (i = emsg->index; i < emsg->end_index - 4; i++)
    {
        //cout << "i: " << i << " :: next index: " << g_next_index << "\n";
        //fflush(stdout);

        TxnManager *tman = get_transaction_manager(i, 0);

        inc_next_index();
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

        inc_next_index();
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

    inc_next_index();
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
    crsp->copy_from_txn(txn_man);
// #if NARWHAL
    double tend = simulation->seconds_from_start(get_sys_clock());

    usleep((g_node_cnt)*(uint)(1000000*(tend-tstart)));
// #endif
    vector<uint64_t> dest;
    dest.push_back(txn_man->client_id);
    msg_queue.enqueue(get_thd_id(), crsp, dest);
    dest.clear();

    INC_STATS(get_thd_id(), txn_cnt, 1);
    INC_STATS(_thd_id, tput_msg, 1);
    INC_STATS(_thd_id, msg_cl_out, 1);

    // Check and Send checkpoint messages.
    send_checkpoints(txn_man->get_txn_id());

    // Setting the next expected prepare message id.
    set_expectedExecuteCount(get_batch_size() + msg->txn_id);
    // End the execute counter.
    INC_STATS(get_thd_id(), time_execute, get_sys_clock() - ctime);

    return RCOK;
}

RC WorkerThread::process_hotstuff_new_view(Message *msg){
    #if PROCESS_PRINT
    cout << "HOTSTUFF_NEW_VIEW_MSG: TID: " << msg->txn_id << " THD:" << get_thd_id() << " FROM: " << msg->return_node_id << endl;
    fflush(stdout);
    #endif
    // Check if the incoming message is valid.
    HOTSTUFFNewViewMsg *nvmsg = (HOTSTUFFNewViewMsg *)msg;
    #if ENABLE_ENCRYPT
    validate_msg(nvmsg);
    #endif

    // If the 2f+1 vote msgs have arrived
    if(hotstuff_new_viewed(nvmsg)){
#if CHAINED
        if(!txn_man->get_hash().empty())
#else
        if(txn_man->is_committed())
#endif
        {
            advance_view();
        }
    }
    return RCOK;
}

void WorkerThread::advance_view(bool update){
    uint64_t view = get_current_view(0) + 1;
    cout << "SET_VIEW[" << view << "]" << endl;
    for(uint64_t i=0; i<g_total_thread_cnt; i++){
        set_view(i, view);
    }
    if(g_node_id == get_view_primary(view))
    {
#if CHAINED
        if(update){
            string hash = txn_man->get_hash();
            txn_man->genericQC.batch_hash = hash;
            txn_man->genericQC.type = PREPARE;
        #if !MUL
            txn_man->genericQC.viewNumber =  get_g_preparedQC().genesis? 0:view - 1;
            txn_man->genericQC.parent_hash = get_g_preparedQC().batch_hash;
            hash_QC_lock.lock();
            hash_to_QC[hash] = txn_man->genericQC;
            hash_QC_lock.unlock();
            update_lockQC(txn_man->genericQC, view);
        #else
            uint64_t instance_id = txn_man->get_txn_id() / get_batch_size() % get_totInstances();
            txn_man->genericQC.viewNumber =  get_g_preparedQC(instance_id).genesis? 0:view - 1;
            txn_man->genericQC.parent_hash = get_g_preparedQC(instance_id).batch_hash;
            hash_QC_lock[instance_id].lock();
            hash_to_QC[instance_id][hash] = txn_man->genericQC;
            hash_QC_lock[instance_id].unlock();
            update_lockQC(txn_man->genericQC, view, instance_id);
        #endif
        }

#endif

        set_curr_new_viewed(txn_man->get_txn_id());
        set_sent(false);

        #if SEMA_TEST
        // Having received enough new_view msgs, the replica becomes the primary in HOTSTUFF (one primary in MUL)
        sem_post(&new_txn_semaphore);
        #endif

    }
}


#if CHAINED

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

#if !MUL
    if(get_view_primary(get_current_view(get_thd_id())) != gene->return_node_id){
        printf("Incorrect Leader %lu in this view %lu\n", gene->return_node_id, get_current_view(get_thd_id()));
#else
    uint64_t instance_id = gene->txn_id / get_batch_size() % get_totInstances();
    if(get_view_primary(get_current_view(instance_id), instance_id) != gene->return_node_id){
        printf("[E]Incorrect Leader %lu in this view %lu in instance %lu\n", gene->return_node_id, get_current_view(instance_id), instance_id);
#endif  
        return RCOK;
    }

    // Check if the message is valid.
    validate_msg(gene);

#if !MUL
    if(!SafeNode(gene->highQC)){
        cout << "UnSafe Node" << endl;
#else
    if(!SafeNode(gene->highQC, instance_id)){
        cout << "UnSafe Node in Instance " << instance_id << endl;
#endif
        return RCOK;
    }

    // Allocate transaction managers for all the transactions in the batch.

    set_txn_man_fields(gene, 0);
    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(gene);

    uint64_t txnid = txn_man->get_txn_id();
    string hash = txn_man->get_hash();
#if !MUL
    hash_QC_lock.lock();
    hash_to_txnid.insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_to_view.insert(make_pair<string&,uint64_t&>(hash, gene->view));
    hash_to_QC[gene->highQC.batch_hash] = gene->highQC; 
    hash_QC_lock.unlock();
    update_lockQC(gene->highQC, gene->view);
#else
    hash_QC_lock[instance_id].lock();
    hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_to_view[instance_id].insert(make_pair<string&,uint64_t&>(hash, gene->view));
    hash_to_QC[instance_id][gene->highQC.batch_hash] = gene->highQC;
    hash_QC_lock[instance_id].unlock();
    update_lockQC(gene->highQC, gene->view, instance_id);
#endif

    create_and_send_narwhal_payload(gene);
    txn_man->narwhal_lock.lock();
    txn_man->narwhal_count++;
    // cout << "NC: "  << txn_man->narwhal_count << endl;
    if(txn_man->narwhal_count == 2 * g_min_invalid_nodes + 1){
        cout << "Here2 "  << txn_man->get_txn_id() << endl;
        sem_post(&narwhal_signal);
    }
    txn_man->narwhal_lock.unlock();
    if(!txn_man->send_hotstuff_newview()){
        if(txn_man->is_new_viewed()){
            advance_view();
        }
    }else{
        // printf("[S]\n");
        // fflush(stdout);
        advance_view(false);
    }

    // Release this txn_man for other threads to use.
    bool ready = txn_man->set_ready();
    assert(ready);
    // UnSetting the ready for the txn id representing this batch.
    txn_man = get_transaction_manager(msg->txn_id, 0);
    unset_ready_txn(txn_man);
    sem_wait(&narwhal_signal);

    return RCOK;
}

#endif

RC WorkerThread::process_narwhal_payload(Message *msg)
{
    //uint64_t cntime = get_sys_clock();
    NarwhalPayloadMsg* gene = (NarwhalPayloadMsg *)msg;
#if PROCESS_PRINT
    printf("NARWHAL PAYLOAD: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",gene->txn_id, gene->view, get_thd_id(), gene->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif
    // Check if the message is valid.
    for(uint64_t i=0; i<2*g_min_invalid_nodes+1; i++){
        validate_msg(gene);
    }
    // Allocate transaction managers for all the transactions in the batch.
    read_txns(gene, 0);
    return RCOK;
}

//Here
void WorkerThread::create_and_send_narwhal_payload(HOTSTUFFGenericMsg *msg){

    Message *bmsg = Message::create_message(NARWHAL_PAYLOAD_MSG);
    NarwhalPayloadMsg *prep = (NarwhalPayloadMsg *)bmsg;

    prep->init(0);

    // String of transactions in a batch to generate hash.
    string batchStr;
    // Allocate transaction manager for all the requests in batch.
    uint64_t start_id = msg->txn_id + 3 - get_batch_size();
    for (uint64_t i = 0; i < get_batch_size() - 1; i++)
    {
        uint64_t txn_id = start_id + i;

    	// TODO: Some memory is getting consumed while storing client query.
        // cout << "A1" << endl;
        auto msg_ = Message::create_message(CL_QRY);
        auto clqry = (YCSBClientQueryMessage*)msg_;
        auto ycsb = new ycsb_request;
        ycsb->key = 1;
        ycsb->value = 1;
        // cout << "A2" << endl;
        clqry->requests.init(1);
        clqry->requests.add(ycsb);
        // // cout << "A3" << endl;
	    // char *bfr = (char *)malloc(clqry->get_size());
	    // clqry->copy_to_buf(bfr);
	    // Message *tmsg = Message::create_message(bfr);
	    // YCSBClientQueryMessage *yqry = (YCSBClientQueryMessage *)tmsg;
	    // free(bfr);
        // delete ycsb;
        // delete clqry;
        // cout << "A4" << endl;
	    prep->add_request_msg(i, clqry);
        // cout << "A5" << endl;
	    prep->index.add(txn_id);

    }

    uint64_t txn_id = start_id + get_batch_size() - 1;
    auto msg_ = Message::create_message(CL_QRY);
    auto clqry = (YCSBClientQueryMessage*)msg_;
    auto ycsb = new ycsb_request;
    ycsb->key = 1;
    ycsb->value = 1;
    clqry->requests.init(1);
    clqry->requests.add(ycsb);
	// char *bfr = (char *)malloc(clqry->get_size());
	// clqry->copy_to_buf(bfr);
	// Message *tmsg = Message::create_message(bfr);
	// YCSBClientQueryMessage *yqry = (YCSBClientQueryMessage *)tmsg;
	// free(bfr);
    // delete ycsb;
    // delete clqry;
	prep->add_request_msg(get_batch_size() - 1, clqry);
	prep->index.add(txn_id);
    
    txn_man = get_transaction_manager(txn_id, 0);

    prep->copy_from_txn(txn_man);
    prep->txn_id = msg->txn_id;
    prep->highQC = QuorumCertificate(0);

    // Send the HOTSTUFFPrepareMsg message to all the other replicas.
    vector<uint64_t> dest = nodes_to_send(0, g_node_cnt);
    // cout << "[X5]" << endl;
    // fflush(stdout);
    msg_queue.enqueue(get_thd_id(), prep, dest);
    // cout << "[X7]" << endl;
    // fflush(stdout);
    dest.clear();
} 


#endif