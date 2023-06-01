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
#if SEPARATE
    Message *bmsg = Message::create_message(PVP_PROPOSAL_MSG);
    PVPProposalMsg *prep = (PVPProposalMsg *)bmsg;
#else
    Message *bmsg = Message::create_message(PVP_GENERIC_MSG);
    PVPGenericMsg *prep = (PVPGenericMsg *)bmsg;
#endif
    
    uint64_t instance_id = msg->txn_id % get_totInstances();
    prep->init(instance_id);

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

    hash_QC_lock[instance_id].lock();
    hash_to_txnid[instance_id][txn_man->get_hash()] = txn_man->get_txn_id();
    hash_QC_lock[instance_id].unlock();

    txn_man->hashSize = txn_man->hash.length();

    prep->copy_from_txn(txn_man);

    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prep);
    txn_man->proposal_received = true;

    // Send the HOTSTUFFPrepareMsg message to all the other replicas.
    vector<uint64_t> dest = nodes_to_send(0, g_node_cnt);
#if DARK_TEST
    if(g_node_id % DARK_FREQ == DARK_ID && g_node_id / DARK_FREQ < DARK_CNT){
        dest.clear();
        for(uint64_t i=0; i<g_node_cnt; i++){
            if(i==g_node_id){
                continue;
            }
            if(i % DARK_FREQ == VICTIM_ID){
                continue;
            }
            dest.push_back(i);
        }
    }
#endif
    msg_queue.enqueue(get_thd_id(), prep, dest);
    dest.clear();
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
    

#if EQUIV_TEST
    if(g_node_id % EQUIV_FREQ == EQUIV_ID && g_node_id / EQUIV_FREQ < EQUIV_CNT){
        equivocate(clbtch, clbtch->txn_id);
    }
    else{
        create_and_send_hotstuff_prepare(clbtch, clbtch->txn_id);
    }
#else
    // Initialize all transaction mangers and Send Prepare message.
    create_and_send_hotstuff_prepare(clbtch, clbtch->txn_id);
#endif
    return RCOK;
}

RC WorkerThread::process_hotstuff_execute(Message *msg){
    #if PROCESS_PRINT
    cout << "EXECUTE " << msg->txn_id << " THREAD: " << get_thd_id() << "\n";
    fflush(stdout);
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



    txn_man = get_transaction_manager(emsg->txn_id + 1, 0);
#if EQUIV_TEST
    while(!txn_man->executable){
        sem_wait(&executable_signal);
    }
#endif

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
    printf("PVP_SYNC_MSG: TID: %lu THD: %lu FROM: %lu\n", msg->txn_id, get_thd_id(), msg->return_node_id);
    fflush(stdout);
    #endif
    // Check if the incoming message is valid.
    PVPSyncMsg *nvmsg = (PVPSyncMsg *)msg;
    #if ENABLE_ENCRYPT
    validate_msg(nvmsg);
    #endif
    // #if PROCESS_PRINT
    // printf("PVP_SYNC_MSG: TID: %lu THD: %lu FROM: %lu VIEW: %lu\n", msg->txn_id, get_thd_id(), msg->return_node_id, nvmsg->view);
    // fflush(stdout);
    // #endif
    if(!txn_man->get_hash().empty()){
        if(nvmsg->hash != txn_man->get_hash()){
            // cout << "[INEQUAL HASH]" << endl;
            return RCOK;
        }
    }

    // If the 2f+1 vote msgs have arrived
    if(hotstuff_new_viewed(nvmsg)){

        // cout << "[XX]" << txn_man->proposal_received << txn_man->generic_received << endl;
#if SEPARATE
        if(txn_man->proposal_received && txn_man->generic_received)
// #else
//         if(!txn_man->get_hash().empty())
#endif
        {
            advance_view();
        }
    }
    // f+1 vote msgs have arrived
#if ENABLE_ASK
#if DARK_TEST
    else if(txn_man->new_view_vote_cnt == fp1 && !txn_man->proposal_received){

        if(g_node_id % DARK_FREQ != VICTIM_ID){
            return RCOK;
        }
        txn_man->instance_id = nvmsg->instance_id;
        txn_man->send_pvp_ask(nvmsg);
        if(!SafeNode(nvmsg->highQC, txn_man->instance_id)){
            cout << "UnSafe Node in Instance " << txn_man->instance_id << endl;
            return RCOK;
        }

        txn_man->send_hotstuff_newview(nvmsg);
    }
#endif
#endif

#if ENABLE_ASK
    else if(txn_man->new_view_vote_cnt == fp1){
#if EQUIV_TEST
        if(g_node_id % EQUIV_FREQ != EQ_VICTIM_ID){
            return RCOK;
        }

        // if(nvmsg->txn_id == 999){
        //     cout << "[1]" << nvmsg->highQC.viewNumber << endl;
        //     cout << "[2]" << get_g_preparedQC(nvmsg->instance_id).viewNumber  << endl;
        //     cout << "[3]" << get_g_preparedQC(nvmsg->instance_id).genesis << endl;
        //     cout << "[4]" << nvmsg->highQC.genesis << endl;
        // }

        if(nvmsg->highQC.viewNumber > get_g_preparedQC(nvmsg->instance_id).viewNumber 
        || get_g_preparedQC(nvmsg->instance_id).genesis && !nvmsg->highQC.genesis){
            
            hash_QC_lock[nvmsg->instance_id].lock();
            uint64_t tid = hash_to_txnid[nvmsg->instance_id][nvmsg->highQC.batch_hash];
            // printf("Here%lu %lu\n", nvmsg->txn_id, tid);
            hash_QC_lock[nvmsg->instance_id].unlock();
            TxnManager *tman = get_transaction_manager(tid, 0);
            tman->executable = false;
            set_g_preparedQC(nvmsg->highQC, nvmsg->instance_id, tid);
            update_lockQC(nvmsg->highQC, nvmsg->highQC.viewNumber, tid, nvmsg->instance_id);
            txn_man->instance_id = tman->instance_id = nvmsg->instance_id;
            tman->send_pvp_ask(nvmsg);
            skip_view(nvmsg, tid);
            txn_man->send_hotstuff_newview(nvmsg);
        }
#endif
    }
#endif

    return RCOK;
}


#if !SEPARATE
/**
 * Process incoming PVPGenericMsg message from the Primary.
 *
 * @param msg Batch of Transactions of type HOTSTUFF_PREP_MSG from the primary.
 * @return RC
 */
RC WorkerThread::process_hotstuff_generic(Message *msg)
{
    //uint64_t cntime = get_sys_clock();
    PVPGenericMsg* gene = (PVPGenericMsg *)msg;
#if PROCESS_PRINT
    printf("PVP_GENERIC_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld  %lf\n",gene->txn_id, gene->view, get_thd_id(), gene->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif


    uint64_t instance_id = gene->instance_id;
    
    if(get_view_primary(get_current_view(instance_id), instance_id) != gene->return_node_id){
        printf("[E1]Incorrect Leader %lu in this view %lu in instance %lu\n", gene->return_node_id, get_current_view(instance_id), instance_id);
        return RCOK;
    }

    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(gene);
#endif

    if(!SafeNode(gene->highQC, instance_id)){
        cout << "UnSafe Node in Instance " << instance_id << endl;
        return RCOK;
    }

    // Allocate transaction managers for all the transactions in the batch.
    set_txn_man_fields(gene, 0);

    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(gene);

    uint64_t txnid = txn_man->get_txn_id();
    string hash = txn_man->get_hash();
    hash_QC_lock[instance_id].lock();
    txnid_to_hash[instance_id].insert(make_pair<uint64_t&,string&>(txnid, hash));
    hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_to_QC[instance_id][gene->highQC.batch_hash] = gene->highQC;
    hash_QC_lock[instance_id].unlock();
    uint64_t view_number = gene->txn_id / get_batch_size() / get_totInstances();
    if(gene->highQC.viewNumber > get_g_preparedQC(instance_id).viewNumber){
        cout << "NM" << endl;
        update_lockQC(gene->highQC, view_number, txnid - get_totInstances()*get_batch_size(), instance_id);
    }
    
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
    PVPProposalMsg* prop = (PVPProposalMsg *)msg;
#if PROCESS_PRINT
    printf("PVP_PROPOSAL_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",prop->txn_id, prop->view, get_thd_id(), prop->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif
    txn_man = get_transaction_manager(msg->txn_id + 2, 0);

    if(txn_man->proposal_received){
        txn_man = nullptr;
        return RCOK;
    }else if(txn_man->generic_received){
        if(prop->hash != txn_man->get_hash()){
            assert(false);
            return RCOK;
        }
    }

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

    txn_man->proposal_received = true;
#if ENABLE_ASK
    if(txn_man->waiting_ask_resp){
        txn_man->waiting_ask_resp = false;
        send_ask_response(txn_man->waiting_ask_resp_id, txn_man->get_txn_id() + 1 - get_batch_size(), get_current_view(instance_id));
    }
#endif

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
    PVPGenericMsg* gene = (PVPGenericMsg *)msg;
#if PROCESS_PRINT
    printf("PVP_GENERIC_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",gene->txn_id, gene->view, get_thd_id(), gene->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif

    if(txn_man->generic_received){
        return RCOK;
    }

    uint64_t instance_id = gene->instance_id;

    if(gene->view > get_current_view(instance_id)){
        skip_view(gene);
    }

    if(get_view_primary(get_current_view(instance_id), instance_id) != gene->return_node_id){
        printf("[E2]Incorrect Leader %lu in this view %lu in instance %lu\n", gene->return_node_id, get_current_view(instance_id), instance_id);
        return RCOK;
    }

    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(gene);
#endif

    if(!SafeNode(gene->highQC, instance_id)){
        cout << "UnSafe Node in Instance " << instance_id << endl;
        return RCOK;
    }

#if TIMER_MANAGER
    timer_manager[get_thd_id()].endTimer(instance_id);
#endif

    if(txn_man->get_hash().empty()){
        txn_man->set_hash(gene->hash);
        txn_man->view = gene->view;
        txn_man->instance_id = gene->instance_id;
    }else{
        if(txn_man->get_hash() != gene->hash){
            assert(false);
            return RCOK;
        }
    }

    txn_man->highQC = gene->highQC;
    txn_man->generic_received = true;

    uint64_t txnid = txn_man->get_txn_id();

    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][gene->highQC.batch_hash] = gene->highQC;
    hash_QC_lock[instance_id].unlock();
    
    if(gene->highQC.viewNumber > get_g_preparedQC(instance_id).viewNumber)
        update_lockQC(gene->highQC, gene->view, txnid - get_totInstances() * get_batch_size(), instance_id);

    txn_man->send_hotstuff_newview();
    if(txn_man->is_new_viewed() && txn_man->proposal_received)
        advance_view();

    return RCOK;
}

RC WorkerThread::process_hotstuff_generic_p(Message *msg){
    // printf("[GP]%lu\n", msg->txn_id);
#if EQUIV_TEST
    if(g_node_id % EQUIV_FREQ == EQUIV_ID && g_node_id / EQUIV_FREQ < EQUIV_CNT){
        txn_man->equivocate_generic();
        txn_man->generic_received = true;
        txn_man->equivocate_hotstuff_newview(false);
        txn_man->equivocate_hotstuff_newview(true);
        return RCOK;
    }
#endif
    txn_man->send_hotstuff_generic();
    txn_man->generic_received = true;
    txn_man->send_hotstuff_newview();
    return RCOK;
}

#endif

void WorkerThread::advance_view(){
    uint64_t instance_id = txn_man->instance_id;
    uint64_t view = get_current_view(instance_id);
    // printf("[AV]%lu, %lu\n", instance_id, view);
    uint64_t txn_id = txn_man->get_txn_id();
    view++;

    string hash = txn_man->get_hash();
    txn_man->genericQC.batch_hash = hash;
    assert(!hash.empty());
    txn_man->genericQC.type = PREPARE;
    txn_man->genericQC.viewNumber =  txn_man->highQC.genesis? 0:view - 1;
    txn_man->genericQC.parent_hash = txn_man->highQC.batch_hash;
    txn_man->genericQC.parent_view = txn_man->highQC.viewNumber;
    txn_man->genericQC.grand_hash = txn_man->highQC.parent_hash;
    txn_man->genericQC.grand_view = txn_man->highQC.parent_view;
    // txn_man->genericQC.height = txn_man->highQC.height + 1;
    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][hash] = txn_man->genericQC;
    hash_QC_lock[instance_id].unlock();        
    update_lockQC(txn_man->genericQC, view, txn_id, instance_id);

    set_view(instance_id, view);
    // printf("jj[%lu] = %lu\n", instance_id, view);
    set_curr_new_viewed(txn_id, instance_id);
    
    if(g_node_id == get_view_primary(view, instance_id))
    {
        #if SEPARATE
            sem_post(&new_txn_semaphore);
        #endif
    }else{
        #if TIMER_MANAGER
        timer_manager[get_thd_id()].setTimer(instance_id);
        #endif
    }
    if(g_node_id == get_view_primary(view + ROUNDS_IN_ADVANCE, instance_id)){ 
        #if SEPARATE
        #if PROPOSAL_THREAD
            sem_post(&proposal_semaphore);
        #else
            sem_post(&new_txn_semaphore);
        #endif
        #endif
    }

}

void WorkerThread::skip_view(PVPGenericMsg* gene){
    uint64_t instance_id = gene->instance_id;
    uint64_t view = gene->view;
    uint64_t txn_id = txn_man->get_txn_id() - get_batch_size() * MULTI_INSTANCES;
    string hash = gene->highQC.batch_hash;
    TxnManager* tman = get_transaction_manager(txn_id, 0);
    tman->genericQC = gene->highQC;
    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][hash] = tman->genericQC;
    hash_QC_lock[instance_id].unlock();    
    update_lockQC(tman->genericQC, gene->highQC.viewNumber, txn_id, instance_id);
    set_view(instance_id, view);
    set_curr_new_viewed(txn_id, instance_id);
}

void WorkerThread::skip_view(PVPSyncMsg* nvmsg, uint64_t txnid){
    uint64_t instance_id = nvmsg->instance_id;
    uint64_t view = nvmsg->view;
    set_view(instance_id, view);
    set_curr_new_viewed(txnid, instance_id);
}

#if TIMER_MANAGER

void WorkerThread::send_failed_new_view(uint64_t instance_id, uint64_t view){
    printf("FAILED_NEW_VIEW %lu, %lu\n", instance_id, view);
    Message *msg = Message::create_message(PVP_SYNC_MSG);
    PVPSyncMsg *nvmsg = (PVPSyncMsg*)msg;
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

void WorkerThread::process_failed_new_view(PVPSyncMsg *msg){
    if(fault_manager.sufficient_voters(msg->instance_id, msg->view, msg->return_node_id)){
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

    if(g_node_id == get_view_primary(view, instance_id))
    {
        #if SEPARATE
            sem_post(&new_txn_semaphore);
        #endif
    }else{
        timer_manager[get_thd_id()].setTimer(instance_id);
    } 
    if(g_node_id == get_view_primary(view + ROUNDS_IN_ADVANCE, instance_id)){ 
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



RC WorkerThread::process_pvp_ask(Message *msg){
#if DARK_TEST
    if(g_node_id % DARK_FREQ == DARK_ID && g_node_id / DARK_FREQ < DARK_CNT){
        return RCOK;
    }
#endif

    PVPAskMsg* ask = (PVPAskMsg *)msg;
#if PROCESS_PRINT
    printf("PVP_ASK_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n", 
        ask->txn_id, ask->view, get_thd_id(), ask->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif

#if ENABLE_ASK
    if(!txn_man->proposal_received){
        txn_man->waiting_ask_resp = true;
        txn_man->waiting_ask_resp_id = ask->return_node_id;
        cout << "NOTHING" << endl;
        return RCOK;
    }
#endif

    if(txn_man->get_hash() == ask->hash){
        send_ask_response(ask->return_node_id, msg->txn_id + 1 - get_batch_size(), ask->view);
    }
    return RCOK;
}

RC WorkerThread::process_pvp_ask_response(Message *msg){
        //uint64_t cntime = get_sys_clock();
    
    PVPAskResponseMsg* ares = (PVPAskResponseMsg *)msg;
#if PROCESS_PRINT
    printf("PVPAskResponseMsg: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",
     ares->txn_id, ares->view, get_thd_id(), ares->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif
    
    txn_man = get_transaction_manager(msg->txn_id+2, 0);
    #if EQUIV_TEST
    if(txn_man->proposal_received && txn_man->executable){
    #else
    if(txn_man->proposal_received){
    #endif
        txn_man = nullptr;
        return RCOK;
    }


    uint64_t instance_id = ares->instance_id;
    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(ares);
#endif
    // Allocate transaction managers for all the transactions in the batch.
#if EQUIV_TEST
    bool is_equi_victim = !(txn_man->executable);
    set_txn_man_fields(ares, 0, is_equi_victim);
#else
    set_txn_man_fields(ares, 0);
#endif

    
    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(ares);
    uint64_t txnid = txn_man->get_txn_id();
    string hash = txn_man->get_hash();
    #if EQUIV_TEST
    txn_man->executable = true;
    #endif
    txn_man->proposal_received = txn_man->generic_received = true;
    hash_QC_lock[instance_id].lock();
    txnid_to_hash[instance_id].insert(make_pair<uint64_t&,string&>(txnid, hash));
    hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(hash, txnid));
    hash_QC_lock[instance_id].unlock();

    if(txn_man->is_new_viewed()){
        cout << "[R0]" << msg->txn_id << endl;
        if(txn_man->highQC.viewNumber > get_g_preparedQC(instance_id).viewNumber){
            cout << "[R1]" << endl;
            update_lockQC(txn_man->highQC, ares->view, txnid - get_totInstances() * get_batch_size(), instance_id);
        }
        advance_view();
        cout << "[R2]" << endl;
    }

    // Release this txn_man for other threads to use.
    bool ready = txn_man->set_ready();
    assert(ready);
    // UnSetting the ready for the txn id representing this batch.
    txn_man = get_transaction_manager(msg->txn_id, 0);
    unset_ready_txn(txn_man);
    return RCOK;
}

RC WorkerThread::send_ask_response(uint64_t dest_node, uint64_t start_id, uint64_t view){
    #if PROCESS_PRINT
    printf("[SEND ASK RESP]TID: %lu THD: %lu \n", start_id + get_batch_size() - 1, get_thd_id());
    #endif
    Message *msg = Message::create_message(txn_man, PVP_ASK_RESPONSE_MSG);
    PVPAskResponseMsg *ares = (PVPAskResponseMsg *)msg;
    ares->view = view;
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        TxnManager* tman = get_transaction_manager(start_id+i, 0);
        ares->copy_from_txn_manager(tman);
    }
    vector<uint64_t> dest;
    dest.push_back(dest_node);
    msg_queue.enqueue(get_thd_id(), ares, dest);
    dest.clear();
    return RCOK;
}

#if EQUIV_TEST

void WorkerThread::equivocate(ClientQueryBatch *msg, uint64_t tid){
    Message *bmsg = Message::create_message(PVP_PROPOSAL_MSG);
    PVPProposalMsg *prep = (PVPProposalMsg *)bmsg;

    Message *bmsg2 = Message::create_message(PVP_PROPOSAL_MSG);
    PVPProposalMsg *prep2 = (PVPProposalMsg *)bmsg2;
    
    uint64_t instance_id = msg->txn_id % get_totInstances();
    prep->init(instance_id);
    prep2->init(instance_id);

    // Starting index for this batch of transactions.
    next_set = tid;

    // String of transactions in a batch to generate hash.
    string batchStr;
    string batchStr2;
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
        if(i < get_batch_size() - 1){
            batchStr2 += msg->cqrySet[i]->getString();
        }

        // Setting up data for HOTSTUFFPrepareMsg Message.
        prep->copy_from_txn(txn_man, msg->cqrySet[i]);
        if(i == get_batch_size() - 1){

            YCSBClientQueryMessage* cqry = msg->cqrySet[i];
            cqry->requests[0]->key = 0;
            cqry->requests[0]->value = 0;
            batchStr2 += cqry->getString();
            prep2->copy_from_txn(txn_man, cqry);
        }else{
            prep2->copy_from_txn(txn_man, msg->cqrySet[i]);
        }

        // Reset this txn manager.
        bool ready = txn_man->set_ready();
        assert(ready);
    }

    // Now we need to unset the txn_man again for the last txn of batch.
    unset_ready_txn(txn_man);

    // Generating the hash representing the whole batch in last txn man.
    txn_man->set_hash(calculateHash(batchStr));
    txn_man->hash2 = calculateHash(batchStr2);
    assert(!txn_man->get_hash().empty());

    hash_QC_lock[instance_id].lock();
    hash_to_txnid[instance_id][txn_man->get_hash()] = txn_man->get_txn_id();
    hash_QC_lock[instance_id].unlock();

    txn_man->hashSize = txn_man->hash.length();
    txn_man->hashSize2 = txn_man->hash2.length();

    prep->copy_from_txn(txn_man);
    prep2->copy_from_txn(txn_man);
    prep2->hash = txn_man->hash2;

    // Storing the HOTSTUFFPrepareMsg message.
    txn_man->set_primarybatch(prep);    
    txn_man->proposal_received = true;

    // Send the HOTSTUFFPrepareMsg message to all the other replicas.
    vector<uint64_t> dest;
    vector<uint64_t> dest2;
    for(uint64_t i = 0; i < g_node_cnt; i++){
        if(i==g_node_id){
            continue;
        }else if(i % EQUIV_FREQ == EQ_VICTIM_ID){
            dest2.push_back(i);
        }else{
            dest.push_back(i);
        }
    }
    msg_queue.enqueue(get_thd_id(), prep, dest);
    msg_queue.enqueue(get_thd_id(), prep2, dest2);
    dest.clear();
    dest2.clear();
} 


RC WorkerThread::process_equivocate_generic(Message *msg){
    PVPGenericMsg* gene = (PVPGenericMsg *)msg;
#if PROCESS_PRINT
    printf("[EQ]PVP_GENERIC_MSG: TID:%ld : VIEW: %ld THD: %ld  FROM: %ld   %lf\n",gene->txn_id, gene->view, get_thd_id(), gene->return_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
#endif

    uint64_t instance_id = gene->instance_id;
    if(get_view_primary(get_current_view(instance_id), instance_id) != gene->return_node_id){
        printf("[E3]Incorrect Leader %lu in this view %lu in instance %lu\n", gene->return_node_id, get_current_view(instance_id), instance_id);
        return RCOK;
    }

    // Check if the message is valid.
#if ENABLE_ENCRYPT
    validate_msg(gene);
#endif

    assert(gene->hash.size() == gene->hashSize);
    assert(gene->hash.size() == 64);

    string hash2 = gene->hash.substr(gene->hashSize/2, gene->hashSize/2);
    gene->hash = gene->hash.substr(0, gene->hashSize/2);
    gene->hashSize /= 2;

    // cout << "[X]" << gene->hash << endl;
    // cout << "[Y]" << hash2 << endl;
    // fflush(stdout);

    if(!SafeNode(gene->highQC, instance_id)){
        cout << "UnSafe Node in Instance " << instance_id << endl;
        return RCOK;
    }

    txn_man->set_hash(gene->hash);
    txn_man->view = gene->view;
    txn_man->instance_id = gene->instance_id;
    txn_man->hash2 = hash2;
    txn_man->hashSize2 = hash2.size();
    fflush(stdout);
    // assert(txn_man->hashSize2 == 32);
    
    // else{
    //     if(txn_man->get_hash() != gene->hash){
    //         assert(false);
    //         return RCOK;
    //     }
    // }

    txn_man->highQC = gene->highQC;
    txn_man->generic_received = true;

    uint64_t txnid = txn_man->get_txn_id();

    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][gene->highQC.batch_hash] = gene->highQC;
    hash_QC_lock[instance_id].unlock();
    
    if(gene->highQC.viewNumber > get_g_preparedQC(instance_id).viewNumber)
        update_lockQC(gene->highQC, gene->view, txnid - get_totInstances() * get_batch_size(), instance_id);

    txn_man->equivocate_hotstuff_newview(false);
    txn_man->equivocate_hotstuff_newview(true);

    if(txn_man->is_new_viewed() && txn_man->proposal_received)
        advance_view();

    return RCOK;
}
#endif


#endif