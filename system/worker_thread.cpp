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

#if THRESHOLD_SIGNATURE && CONSENSUS == HOTSTUFF

WorkerThread::~WorkerThread() {
     if(txn_man){
         delete txn_man;
         txn_man = nullptr;
     }
 }

void WorkerThread::send_public_key(){
     for (uint64_t i = 0; i < g_node_cnt; i++)
    {
        if (i == g_node_id)
        {
            continue;
        }
        Message *msg = Message::create_message(KEYEX);
        KeyExchange *keyex = (KeyExchange *)msg;
        keyex->pkey = "SECP";
        keyex->public_share = public_key;

        keyex->pkeySz = keyex->pkey.size();
        keyex->return_node = g_node_id;

        vector<uint64_t> dest;
        dest.push_back(i);
        msg_queue.enqueue(get_thd_id(), keyex, dest);
        dest.clear();
    }
}
#endif

void WorkerThread::send_key()
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
        keyex->pkey = "RSA-" + g_public_key;
        #if PRINT_KEYEX
        cout << "Sending RSA: " << g_public_key << endl;
        #endif
#elif CRYPTO_METHOD_ED25519
        keyex->pkey = "ED2-" + g_public_key;
        #if PRINT_KEYEX
        cout << "Sending ED25519: " << g_public_key << endl;
        #endif
#endif
        fflush(stdout);

        keyex->pkeySz = keyex->pkey.size();
        keyex->return_node = g_node_id;

        vector<uint64_t> dest;
        dest.push_back(i);
        msg_queue.enqueue(get_thd_id(), keyex, dest);
        dest.clear();

#endif

#if CRYPTO_METHOD_CMAC_AES
        Message *msgCMAC = Message::create_message(KEYEX);
        KeyExchange *keyexCMAC = (KeyExchange *)msgCMAC;
        keyexCMAC->pkey = "CMA-" + cmacPrivateKeys[i];
        keyexCMAC->pkeySz = keyexCMAC->pkey.size();
        keyexCMAC->return_node = g_node_id;

        msg_queue.enqueue(get_thd_id(), keyexCMAC, dest);
        dest.clear();
        #if PRINT_KEYEX
        cout << "Sending CMAC " << cmacPrivateKeys[i] << endl;
        fflush(stdout);
        #endif
#endif
    }
}
void WorkerThread::unset_ready_txn(TxnManager *tman)
{
    // uint64_t spin_wait_starttime = 0;
    while (true)
    {
        bool ready = tman->unset_ready();
        if (!ready)
        {
            // if (spin_wait_starttime == 0)
            //     spin_wait_starttime = get_sys_clock();
            continue;
        }
        else
        {
            // if (spin_wait_starttime > 0)
            //     INC_STATS(_thd_id, worker_spin_wait_time, get_sys_clock() - spin_wait_starttime);
            break;
        }
    }
}
void WorkerThread::setup()
{
    // Increment commonVar.
    batchMTX.lock();
    commonVar++;
    batchMTX.unlock();

    if (get_thd_id() == 0)
    {
        while (commonVar < g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt)
            ;

        send_init_done_to_all_nodes();
        send_key();
#if THRESHOLD_SIGNATURE
        if(ISSERVER)
            send_public_key();
#endif
    }
    _thd_txn_id = 0;
}

void WorkerThread::process(Message *msg)
{
    RC rc __attribute__((unused));

    switch (msg->get_rtype())
    {
    case KEYEX:
        rc = process_key_exchange(msg);
        break;
    case CL_BATCH:
        #if CONSENSUS == HOTSTUFF
        rc = process_client_batch_hotstuff(msg);
        #else
        rc = process_client_batch(msg);
        #endif
        break;
    case PBFT_CHKPT_MSG:
        rc = process_pbft_chkpt_msg(msg);
        break;
    case EXECUTE_MSG:
        #if CONSENSUS == HOTSTUFF
        rc = process_hotstuff_execute(msg);
        #else
        rc = process_execute_msg(msg);
        #endif
        break;

#if CONSENSUS == HOTSTUFF
    case PVP_SYNC_MSG:
        rc = process_hotstuff_new_view(msg);
        break;
    case PVP_GENERIC_MSG:
#if EQUIV_TEST
        if(g_node_id % EQUIV_FREQ == EQUIV_ID && g_node_id / EQUIV_FREQ < EQUIV_CNT 
         && msg->return_node_id % EQUIV_FREQ == EQUIV_ID && msg->return_node_id / EQUIV_FREQ < EQUIV_CNT){
            rc = process_equivocate_generic(msg);
            break;
        }
#endif
        rc = process_hotstuff_generic(msg);
        break;

#if SEPARATE
    case PVP_PROPOSAL_MSG:
        rc = process_hotstuff_proposal(msg);
        break;
    case PVP_GENERIC_MSG_P:
        rc = process_hotstuff_generic_p(msg);
        break;
#endif

    case PVP_ASK_MSG:
        rc = process_pvp_ask(msg);
        break;
    case PVP_ASK_RESPONSE_MSG:
        rc = process_pvp_ask_response(msg);
        break;


#endif
    default:
        printf("rtype: %d from %ld\n", msg->get_rtype(), msg->return_node_id);
        fflush(stdout);
        assert(false);
        break;
    }
}

RC WorkerThread::process_key_exchange(Message *msg)
{
    KeyExchange *keyex = (KeyExchange *)msg;
    string algorithm = keyex->pkey.substr(0, 4);
    keyex->pkey = keyex->pkey.substr(4, keyex->pkey.size() - 4);
    //cout << "Algo: " << algorithm << " :: " <<keyex->return_node << endl;
    //cout << "Storing the key: " << keyex->pkey << " ::size: " << keyex->pkey.size() << endl;
    fflush(stdout);

#if CRYPTO_METHOD_CMAC_AES
    if (algorithm == "CMA-")
    {
        cmacOthersKeys[keyex->return_node] = keyex->pkey;
        receivedKeys[keyex->return_node]--;
    }
#endif

// When using ED25519 we create the verifier for this pkey.
// This saves some time during the verification
#if CRYPTO_METHOD_ED25519
    if (algorithm == "ED2-")
    {
        //cout << "Key length: " << keyex->pkey.size() << " for ED255: " << CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH << endl;
        g_pub_keys[keyex->return_node] = keyex->pkey;
        byte byteKey[CryptoPP::ed25519PrivateKey::PUBLIC_KEYLENGTH];
        copyStringToByte(byteKey, keyex->pkey);
        verifier[keyex->return_node] = CryptoPP::ed25519::Verifier(byteKey);
        receivedKeys[keyex->return_node]--;
    }

#elif CRYPTO_METHOD_RSA
    if (algorithm == "RSA-")
    {
        g_pub_keys[keyex->return_node] = keyex->pkey;
        receivedKeys[keyex->return_node]--;
    }
#endif

#if THRESHOLD_SIGNATURE
    if (algorithm == "SECP"){
        public_keys[keyex->return_node] = keyex->public_share;
    }
#endif

    bool sendReady = true;
    //Check if we have the keys of every node
    uint64_t totnodes = g_node_cnt + g_client_node_cnt;
    for (uint64_t i = 0; i < totnodes; i++)
    {
        if (receivedKeys[i] != 0)
        {
            sendReady = false;
        }
    }

#if THRESHOLD_SIGNATURE
    if(public_keys.size() < g_node_cnt)
    {
        sendReady = false;
    }
#endif

    if (sendReady)
    {
        // Send READY to clients.
        for (uint64_t i = g_node_cnt; i < totnodes; i++)
        {
            Message *rdy = Message::create_message(READY);

            vector<uint64_t> dest;
            dest.push_back(i);
            msg_queue.enqueue(get_thd_id(), rdy, dest);
            dest.clear();
        }

    }

    return RCOK;
}

void WorkerThread::release_txn_man(uint64_t txn_id, uint64_t batch_id)
{
    txn_table.release_transaction_manager(get_thd_id(), txn_id, batch_id);
}

TxnManager *WorkerThread::get_transaction_manager(uint64_t txn_id, uint64_t batch_id)
{
    TxnManager *tman = txn_table.get_transaction_manager(get_thd_id(), txn_id, batch_id);
    return tman;
}

/* Returns the id for the next txn. */
uint64_t WorkerThread::get_next_txn_id()
{
    uint64_t txn_id = get_batch_size() * next_set;
    return txn_id;
}

/**
 * Starting point for each worker thread.
 *
 * Each worker-thread created in the main() starts here. Each worker-thread is alive
 * till the time simulation is not done, and continuousy perform a set of tasks. 
 * Thess tasks involve, dequeuing a message from its queue and then processing it 
 * through call to the relevant function.
 */
RC WorkerThread::run()
{
    tsetup();
    printf("Running WorkerThread %ld\n", _thd_id);
    fflush(stdout);
    uint64_t agCount = 0, ready_starttime, idle_starttime = 0;

    // Setting batch (only relevant for batching threads).
    next_set = 0;

    while (!simulation->is_done())
    {
        txn_man = NULL;
        heartbeat();
        progress_stats();
#if SEMA_TEST
        uint64_t thd_id = get_thd_id();
        idle_starttime = get_sys_clock();
        uint64_t num_multi_threads = get_multi_threads();
        // Wait until there is at least one msg in its corresponding queue
        // printf("[P1]%lu\n", thd_id);
        // if(thd_id != g_thread_cnt-2)
        if(thd_id > num_multi_threads + 3){
            thd_id = num_multi_threads + 3;
        }
        // if(thd_id == num_multi_threads){
        //     cout << "[V1]" << endl;
        //     fflush(stdout);
        // }
        sem_wait(&worker_queue_semaphore[thd_id]);

        // if(thd_id == num_multi_threads){
        //     cout << "[V2]" << endl;
        //     fflush(stdout);
        // }
        // The batch_thread waits until the replica becomes primary of at least one instance 
        if(thd_id == num_multi_threads+1)  {
            sem_wait(&new_txn_semaphore);
        } 

        #if PROPOSAL_THREAD
        else if(thd_id == num_multi_threads)  {
            sem_wait(&proposal_semaphore);
            // cout << "[V3]" << endl;
            // fflush(stdout);
        } 
        #endif
        // The execute_thread waits until the next msg to execute is enqueued
        else if (thd_id == num_multi_threads + 2){
            sem_wait(&execute_semaphore);
        }


#if AUTO_POST
        #if NEW_DIVIDER && FAIL_DIVIDER == 3
		while(!(g_node_id % DIV1 < LIMIT1 && g_node_id % DIV2 != LIMIT2) && thd_id >= 0 && thd_id < get_multi_threads())
		#else
        while(g_node_id % FAIL_DIVIDER != FAIL_ID  && thd_id >= 0 && thd_id < get_multi_threads())
        #endif
        {
            bool timeout = false;
            uint64_t to_id = timer_manager[thd_id].check_timers(timeout);
            if(timeout){
                uint64_t cview = get_current_view(to_id);
                uint64_t value = get_view_primary(cview, to_id);
                #if NEW_DIVIDER && FAIL_DIVIDER == 3
                if(cview >= CRASH_VIEW && value % DIV1 < LIMIT1 && value % DIV2 != LIMIT2){
                #else
                if(cview >= CRASH_VIEW && value % FAIL_DIVIDER == FAIL_ID){
                #endif
                    cout << "TIMEOUT" << to_id << endl;
                    fflush(stdout);
                    send_failed_new_view(to_id, cview);
                }
            }
            if(is_auto_posted(thd_id)){
                set_auto_posted(false, thd_id);
                sem_wait(&worker_queue_semaphore[thd_id]);
            }else{
                break;
            }
        }
#endif

        INC_STATS(_thd_id, worker_idle_time, get_sys_clock() - idle_starttime);
#endif
        // Dequeue a message from its work_queue.
        Message *msg = work_queue.dequeue(get_thd_id());
        if (!msg)
        {
            #if SEMA_TEST
            sem_post(&worker_queue_semaphore[thd_id]);
            #else
            if (idle_starttime == 0)
                idle_starttime = get_sys_clock();
            #endif
            continue;
        }
#if PVP_FAIL
        #if NEW_DIVIDER && FAIL_DIVIDER == 3
        else if(g_node_id % DIV1 < LIMIT1 && g_node_id % DIV2 != LIMIT2 && thd_id >= 0 && thd_id < get_multi_threads() && get_current_view(msg->instance_id) >= CRASH_VIEW){
        #else
        else if(g_node_id % FAIL_DIVIDER == FAIL_ID && thd_id >= 0 && thd_id < get_multi_threads() && get_current_view(msg->instance_id) >= CRASH_VIEW){
        #endif
            Message::release_message(msg);
            continue;
        }
#endif
#if TEMP_QUEUE
        else{
            if(thd_id < get_multi_threads()){
                if(work_queue.check_view(msg)){
                    // if(msg->rtype == PVP_GENERIC_MSG){
                        // cout << "[MD]" << msg->txn_id << endl;
                    // }
                    continue;
                }
            }   
        }
#endif

        #if !SEMA_TEST
        if (idle_starttime > 0)
        {
            INC_STATS(_thd_id, worker_idle_time, get_sys_clock() - idle_starttime);
            idle_starttime = 0;
        }
        #endif

        // Remove redundant messages.
        if (exception_msg_handling(msg))
        {
            continue;
        }

#if CONSENSUS == HOTSTUFF
        if (msg->rtype != PVP_PROPOSAL_MSG && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG && msg->rtype != PVP_ASK_RESPONSE_MSG)
#else
        // Based on the type of the message, we try acquiring the transaction manager.
        if (msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG)
#endif
        {
#if TIMER_MANAGER
            if(msg->rtype == PVP_SYNC_MSG){
                PVPSyncMsg *nvmsg = (PVPSyncMsg*)msg;
                if(nvmsg->non_vote){
                    process_failed_new_view(nvmsg);
                    Message::release_message(msg, 1);
                    INC_STATS(get_thd_id(), worker_release_msg_time, get_sys_clock() - ready_starttime);
                    continue;
                }
            }
#endif
            txn_man = get_transaction_manager(msg->txn_id, 0);
            ready_starttime = get_sys_clock();
            bool ready = txn_man->unset_ready();
            if (!ready)
            {
                // cout << "Placing: Txn: " << msg->txn_id << " Type: " << msg->rtype << "\n";
                // fflush(stdout);
                // Return to work queue, end processing
                work_queue.enqueue(get_thd_id(), msg, true);
                continue;
            }
            txn_man->register_thread(this);
        }
        // Th execut-thread only picks up the next batch for execution.
        if (msg->rtype == EXECUTE_MSG)
        {
            if (msg->txn_id != get_expectedExecuteCount())
            {
                // Return to work queue.
                agCount++;
                work_queue.enqueue(get_thd_id(), msg, true);
                continue;
            }
        }

        if (!simulation->is_warmup_done())
        {
            stats.set_message_size(msg->rtype, msg->get_size());
        }

        process(msg);
        
        ready_starttime = get_sys_clock();
        uint64_t iid = 0;

        if (txn_man)
        {
            iid = txn_man->instance_id;
            bool ready = txn_man->set_ready();
            if (!ready)
            {
                cout << "FAIL: " << msg->txn_id << " :: RT: " << msg->rtype << "\n";
                fflush(stdout);
                assert(ready);
            }
            if(msg->rtype == PBFT_CHKPT_MSG){
                thd_id = get_thd_id();
                g_checkpointing_lock.lock();
                is_chkpt_holding[thd_id % 2] = false;
                if(is_chkpt_stalled[(thd_id+1) % 2]){
                    sem_post(&chkpt_semaphore[(thd_id+1) % 2]);
                }
                g_checkpointing_lock.unlock();
            }
        }
        if(thd_id == num_multi_threads){
            inc_incomplete_proposal_cnt(iid);
        }
        // delete message
        ready_starttime = get_sys_clock();

        Message::release_message(msg, 2);
        INC_STATS(get_thd_id(), worker_release_msg_time, get_sys_clock() - ready_starttime);
    }
#if SEMA_TEST
    for(uint i=0; i<g_thread_cnt; i++){
        sem_post(&worker_queue_semaphore[i]);
    }
    sem_post(&new_txn_semaphore);
    #if PROPOSAL_THREAD
    sem_post(&proposal_semaphore);
    #endif
    sem_post(&execute_semaphore);
    for(uint i=0; i<g_this_send_thread_cnt; i++){
        sem_post(&output_semaphore[i]);
    }
    for(uint i=0; i<get_totInstances(); i++){
        set_sent(false, i);
    }
#endif
    printf("FINISH: %ld %lu\n", agCount, get_thd_id());
    fflush(stdout);

    return FINISH;
}

RC WorkerThread::init_phase()
{
    RC rc = RCOK;
    return rc;
}

bool WorkerThread::is_cc_new_timestamp()
{
    return false;
}

#if BANKING_SMART_CONTRACT
/**
 * This function sets up the required fields of the txn manager.
 *
 * @param clqry One Client Transaction (or Query).
*/
void WorkerThread::init_txn_man(BankingSmartContractMessage *bsc)
{
    txn_man->client_id = bsc->return_node_id;
    txn_man->client_startts = bsc->client_startts;
    SmartContract *smart_contract;
    switch (bsc->type)
    {
    case BSC_TRANSFER:
    {
        TransferMoneySmartContract *tm = new TransferMoneySmartContract();
        tm->amount = bsc->inputs[1];
        tm->source_id = bsc->inputs[0];
        tm->dest_id = bsc->inputs[2];
        tm->type = BSC_TRANSFER;
        smart_contract = (SmartContract *)tm;
        break;
    }
    case BSC_DEPOSIT:
    {
        DepositMoneySmartContract *dm = new DepositMoneySmartContract();

        dm->amount = bsc->inputs[0];
        dm->dest_id = bsc->inputs[1];
        dm->type = BSC_DEPOSIT;
        smart_contract = (SmartContract *)dm;
        break;
    }
    case BSC_WITHDRAW:
    {
        WithdrawMoneySmartContract *wm = new WithdrawMoneySmartContract();
        wm->amount = bsc->inputs[0];
        wm->source_id = bsc->inputs[1];
        wm->type = BSC_WITHDRAW;
        smart_contract = (SmartContract *)wm;
        break;
    }
    default:
        assert(0);
        break;
    }
    txn_man->smart_contract = smart_contract;
}
#else

/**
 * This function sets up the required fields of the txn manager.
 *
 * @param clqry One Client Transaction (or Query).
*/
void WorkerThread::init_txn_man(YCSBClientQueryMessage *clqry, bool is_equi_victim)
{
    txn_man->client_id = clqry->return_node;
    txn_man->client_startts = clqry->client_startts;
    YCSBQuery *query = (YCSBQuery *)(txn_man->query);
    for (uint64_t i = 0; i < clqry->requests.size(); i++)
    {
        ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
        req->key = clqry->requests[i]->key;
        req->value = clqry->requests[i]->value;
        if(is_equi_victim){
            query->requests.set(0, req);
        }else{
            query->requests.add(req);
        }
    }
}
#endif
/**
 * Create an message of type ExecuteMessage, to notify the execute-thread that this 
 * batch of transactions are ready to be executed. This message is placed in one of the 
 * several work-queues for the execute-thread.
 */
void WorkerThread::send_execute_msg()
{
    Message *tmsg = Message::create_message(txn_man, EXECUTE_MSG);
    work_queue.enqueue(get_thd_id(), tmsg, false);
}

/**
 * Execute transactions and send client response.
 *
 * This function is only accessed by the execute-thread, which executes the transactions
 * in a batch, in order. Note that the execute-thread has several queues, and at any 
 * point of time, the execute-thread is aware of which is the next transaction to 
 * execute. Hence, it only loops on one specific queue.
 *
 * @param msg Execute message that notifies execution of a batch.
 * @ret RC
 */
RC WorkerThread::process_execute_msg(Message *msg)
{
    # if PROCESS_PRINT
    if (msg->txn_id / get_batch_size() % 100 == 0)
    {
        cout << "EXECUTE " << msg->txn_id << " THREAD: " << get_thd_id() << "\n";
        fflush(stdout);
    }
    #endif
    uint64_t ctime = get_sys_clock();

    // This message uses txn man of index calling process_execute.

    Message *rsp = Message::create_message(CL_RSP);
    ClientResponseMessage *crsp = (ClientResponseMessage *)rsp;
    crsp->init();

    ExecuteMessage *emsg = (ExecuteMessage *)msg;

    // Execute transactions in a shot
    uint64_t i;
    for (i = emsg->index; i < emsg->end_index - 4; i++)
    {
        //cout << "i: " << i << " :: next index: " << g_next_index << "\n";
        //fflush(stdout);

        TxnManager *tman = get_transaction_manager(i, 0);

        inc_next_index();

        // Execute the transaction
        tman->run_txn();

        // Commit the results.
        tman->commit();

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

        // Execute the transaction
        tman->run_txn();

        // Commit the results.
        tman->commit();

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

    // Execute the transaction
    txn_man->run_txn();

#if ENABLE_CHAIN
    // Add the block to the blockchain.
    BlockChain->add_block(txn_man);
#endif

    // Commit the results.
    txn_man->commit();

    crsp->copy_from_txn(txn_man);

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
/**
 * This function helps in periodically sending out CheckpointMessage. At present these
 * messages are including just including information about first and last txn of the 
 * batch but later we should include a digest. Further, a checkpoint is only sent after
 * transaction id is a multiple of a config.h parameter.
 *
 * @param txn_id Transaction identifier of the last transaction in the batch..
 * @ret RC
 */
void WorkerThread::send_checkpoints(uint64_t txn_id)
{
    if ((txn_id + 1) % txn_per_chkpt() == 0)
    {
        TxnManager *tman = txn_table.get_transaction_manager(get_thd_id(), txn_id, 0);
        tman->send_checkpoint_msgs();
    }
}

/**
 * Checkpoint and Garbage collection.
 *
 * This function waits for 2f+1 messages to mark a checkpoint. Due to different 
 * processing speeds of the replicas, it is possible that a replica may receive 
 * CheckpointMessage from other replicas even before it has finished executing thst 
 * transaction. Hence, we need to be careful when to perform garbage collection.
 * Further, note that the CheckpointMessage messages are handled by a separate thread.
 *
 * @param msg CheckpointMessage corresponding to a checkpoint.
 * @ret RC
 */
RC WorkerThread::process_pbft_chkpt_msg(Message *msg)
{
    // CheckpointMessage *ckmsg = (CheckpointMessage *)msg;
    #if PROCESS_PRINT
    printf("CHKPOINT from %ld:    %ld\n", msg->return_node_id, msg->txn_id);
    fflush(stdout);
    #endif

    // If this checkpoint was already accessed, then return.
    if (txn_man->is_chkpt_ready())
    {
        return RCOK;
    }
    else
    {
        // Check if message is valid.
        // validate_msg(ckmsg);
        uint64_t num_chkpt = txn_man->decr_chkpt_cnt();
        // If sufficient number of messages received, then set the flag.
        if (num_chkpt == 0)
        {
            txn_man->set_chkpt_ready();
        }
        else
        {
            return RCOK;
        }
    }

    // Also update the next checkpoint to the identifier for this message,
    set_curr_chkpt(msg->txn_id);

    // Now we determine what all transaction managers can we release.
    uint64_t del_range = 0;
    
    del_range = msg->get_txn_id();

    del_range -= 16 * get_batch_size() * g_node_cnt;
    
    // Release Txn Managers.
    uint64_t thd_id = get_thd_id();

    g_checkpointing_lock.lock();
    uint64_t start = get_last_deleted_txn();
    uint64_t holding2 = txn_chkpt_holding[(thd_id+1)%2];
    inc_last_deleted_txn(del_range);
    g_checkpointing_lock.unlock();
    
    #if PROCESS_PRINT
    printf("Chkpt: %ld :: LD: %ld :: Del: %ld   Thread:%ld\n",msg->get_txn_id(), start, del_range, get_thd_id());
    fflush(stdout);
    #endif

    for (uint64_t i = start; i < holding2 && i < del_range; i++)
    {   
        release_txn_man(i, 0);
    }
    
    for (uint64_t i = holding2 + 1; i < del_range; i++)
    {   
        release_txn_man(i, 0);
    }
    

    if(holding2 < del_range){
        uint64_t is_stalled = false;
        g_checkpointing_lock.lock();
        if(txn_chkpt_holding[(thd_id+1)%2] == start && is_chkpt_holding[(thd_id+1)%2]){
            is_stalled = is_chkpt_stalled[thd_id % 2] = true;
        }
        g_checkpointing_lock.unlock();
        if(is_stalled){
            sem_wait(&chkpt_semaphore[thd_id % 2]);
            g_checkpointing_lock.lock();
            is_chkpt_stalled[thd_id % 2] = false;
            g_checkpointing_lock.unlock();
        }
        release_txn_man(start, 0);
    }

    return RCOK;
}

/* Specific handling for certain messages. If no handling then return false. */
bool WorkerThread::exception_msg_handling(Message *msg)
{
    if (msg->rtype == KEYEX)
    {
        // Key Exchange message needs to pass directly.
        process(msg);
        Message::release_message(msg);
        return true;
    }
    // Release Messages that arrive after txn completion, except obviously
    // CL_BATCH as it is never late.

    if (msg->rtype != CL_BATCH)
    {
        if(msg->rtype == PVP_ASK_MSG){
            if (msg->txn_id + 1 - get_batch_size() <= get_curr_chkpt())
            {
                cout << "exception msg->txn_id = " << msg->txn_id << endl;
                Message::release_message(msg, 3);
                return true;
            }
            return false;
        }
        if (msg->rtype != PBFT_CHKPT_MSG && msg->rtype != PVP_SYNC_MSG)
        {
            if (msg->txn_id <= curr_next_index())
            {

                // cout << "exception msg->txn_id = " << msg->txn_id << endl;
                Message::release_message(msg, 3);
                return true;
            }
        }
        else if(msg->rtype == PVP_SYNC_MSG){
            uint64_t instance_id = msg->instance_id;
            if (((PVPSyncMsg*)msg)->view < get_current_view(instance_id) ||
             (((PVPSyncMsg*)msg)->view == get_current_view(instance_id) 
             && msg->txn_id <= get_curr_new_viewed(instance_id) && ((PVPSyncMsg*)msg)->non_vote == false))
            {
                Message::release_message(msg, 4);
                return true;
            }
        } 
        else
        {
            if (msg->txn_id <= get_curr_chkpt())
            {
                Message::release_message(msg, 7);
                return true;
            }
        }
    }

    return false;
}

/** UNUSED */
void WorkerThread::algorithm_specific_update(Message *msg, uint64_t idx)
{

}

/**
 * This function is used by the non-primary or backup replicas to create and set 
 * transaction managers for each transaction part of the BatchRequests message sent by
 * the primary replica.
 *
 * @param breq Batch of transactions as a BatchRequests message.
 * @param bid Another dimensional identifier to support more transaction managers.
 */
void WorkerThread::set_txn_man_fields(BatchRequests *breq, uint64_t bid)
{
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        txn_man = get_transaction_manager(breq->index[i], bid);

        unset_ready_txn(txn_man);

        txn_man->register_thread(this);
        txn_man->return_id = breq->return_node_id;

        // Fields that need to updated according to the specific algorithm.
        algorithm_specific_update(breq, i);

        init_txn_man(breq->requestMsg[i]);
        bool ready = txn_man->set_ready();
        assert(ready);
    }

    // We need to unset txn_man again for last txn in the batch.
    unset_ready_txn(txn_man);

    txn_man->set_hash(breq->hash);
}

/** Validates the contents of a message. */
bool WorkerThread::validate_msg(Message *msg)
{
    switch (msg->rtype)
    {
    case KEYEX:
        break;
    case CL_RSP:
        if (!((ClientResponseMessage *)msg)->validate())
        {
            assert(0);
        }
        break;

    case CL_BATCH:
        if (!((ClientQueryBatch *)msg)->validate())
        {
            assert(0);
        }
        break;
    case PBFT_CHKPT_MSG:
        if (!((CheckpointMessage *)msg)->validate())
        {
            assert(0);
        }
        break;
#if CONSENSUS == HOTSTUFF && ENABLE_ENCRYPT
    case PVP_SYNC_MSG:
        if (!((PVPSyncMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
#if SEPARATE
    case PVP_PROPOSAL_MSG:
        if (!((PVPProposalMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
    case PVP_GENERIC_MSG:
        if (!((PVPGenericMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
#else
    case PVP_GENERIC_MSG:
        if (!((PVPGenericMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
#endif
    case PVP_ASK_MSG:
        if (!((PVPAskMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case PVP_ASK_RESPONSE_MSG:
        if (!((PVPAskResponseMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;

#endif
    default:
        break;
    }

    return true;
}

/* Checks the hash and view of a message against current request. */

bool WorkerThread::checkMsg(Message *msg)
{
    uint64_t instance_id = msg->instance_id;
#if CONSENSUS == HOTSTUFF
    if (msg->rtype == PVP_SYNC_MSG){
        PVPSyncMsg *nvmsg = (PVPSyncMsg *)msg;
        if(txn_man->get_hash().compare(nvmsg->hash) == 0) { 
            if(get_current_view(instance_id) == nvmsg->view) {
                return true;
            }
        }
    }
#endif
    return false;
}

#if CONSENSUS == HOTSTUFF

bool WorkerThread::hotstuff_new_viewed(PVPSyncMsg* msg){
    if (txn_man->is_new_viewed())
    {
        return false;
    }
    if(txn_man->vote_new_view.empty()){
        txn_man->vote_new_view.push_back(g_node_id);
    }
    for(uint i=0; i<txn_man->vote_new_view.size(); i++){
        if(msg->return_node_id==txn_man->vote_new_view[i])
            return false;
    }
    if(!msg->sig_empty){
        if(txn_man->get_hash().empty() || txn_man->get_hash() == msg->hash){
            // cout << "[DD]" << msg->return_node_id << endl;
            // fflush(stdout);
            txn_man->genericQC.signature_share_map[msg->return_node_id] = msg->sig_share;
        }
    }
    txn_man->vote_new_view.push_back(msg->return_node_id);
    txn_man->hash_voters[msg->hash].push_back(msg->return_node_id);
    uint64_t val = txn_man->hash_voters[msg->hash].size();
    // cout << "[NN]" << val << endl;
    if(val > txn_man->new_view_vote_cnt){
        txn_man->new_view_vote_cnt = val;
    }

    if(val == 1){
        hash_QC_lock[msg->instance_id].lock();
        hash_to_txnid[msg->instance_id][msg->hash] = msg->txn_id;
        hash_QC_lock[msg->instance_id].unlock();
    }

    if(txn_man->new_view_vote_cnt == nf)
    {
        for(auto& it: txn_man->hash_voters){
            if(it.first != msg->hash){
                for(auto sender: it.second){
                     txn_man->genericQC.signature_share_map.erase(sender);
                }
            }
        }
        txn_man->set_new_viewed();
        return true;
    }

    return false;
}

#if SEPARATE
void WorkerThread::set_txn_man_fields(PVPProposalMsg *prop, uint64_t bid, bool is_equi_victim)
{
    uint64_t instance_id = prop->instance_id;
    uint64_t view = prop->view;
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        txn_man = get_transaction_manager(prop->index[i], bid);
        unset_ready_txn(txn_man);
        txn_man->register_thread(this);
        txn_man->return_id = prop->return_node_id;
        txn_man->view = view;
        txn_man->instance_id = instance_id;
        // Fields that need to updated according to the specific algorithm.
        algorithm_specific_update(prop, i);
        init_txn_man(prop->requestMsg[i], is_equi_victim);
        bool ready = txn_man->set_ready();
        assert(ready);
    }
    // We need to unset txn_man again for last txn in the batch.
    unset_ready_txn(txn_man);
    txn_man->set_hash(prop->hash);
}

#endif

void WorkerThread::send_execute_msg_hotstuff(uint64_t instance_id)
{
    TxnManager* tman;
    Message *tmsg;

    hash_QC_lock[instance_id].lock();
    QuorumCertificate QC = hash_to_QC[instance_id][txn_man->get_hash()];
    QC = hash_to_QC[instance_id][QC.parent_hash];
    hash_QC_lock[instance_id].unlock();

    while(QC.type != COMMIT && !QC.batch_hash.empty()){

        hash_QC_lock[instance_id].lock();
        tman = get_transaction_manager(hash_to_txnid[instance_id][QC.batch_hash], 0);
        hash_QC_lock[instance_id].unlock();

        tmsg = Message::create_message(tman, EXECUTE_MSG);
        cout << "[E1]" << tmsg->txn_id << endl;
        work_queue.enqueue(get_thd_id(), tmsg, false);
        cout << "[E2]" << tmsg->txn_id << endl;

        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][QC.batch_hash].type = COMMIT;
        QC = hash_to_QC[instance_id][QC.parent_hash];
        hash_QC_lock[instance_id].unlock();
    }
    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][txn_man->get_hash()].type = COMMIT;
    hash_QC_lock[instance_id].unlock();  
    send_execute_msg();
}


void WorkerThread::send_execute_msg_hotstuff(TxnManager *t_man, uint64_t instance_id)
{
    TxnManager* tman;
    Message *tmsg;

    hash_QC_lock[instance_id].lock();
    bool exist = true;
    QuorumCertificate QC = hash_to_QC[instance_id][t_man->get_hash()];
    if(hash_to_QC[instance_id].count(QC.parent_hash))
        QC = hash_to_QC[instance_id][QC.parent_hash];
    else{
        exist = false;
    }
    hash_QC_lock[instance_id].unlock();
    while(exist){ 
        hash_QC_lock[instance_id].lock();
        tman = get_transaction_manager(hash_to_txnid[instance_id][QC.batch_hash], 0);
        hash_QC_lock[instance_id].unlock();

        tmsg = Message::create_message(tman, EXECUTE_MSG);
        work_queue.enqueue(get_thd_id(), tmsg, false);

        hash_QC_lock[instance_id].lock();
        
        hash_to_QC[instance_id].erase(QC.batch_hash);
        hash_to_txnid[instance_id].erase(QC.batch_hash);
        txnid_to_hash[instance_id].erase(tman->get_txn_id());
        if(hash_to_QC[instance_id].count(QC.parent_hash))
            QC = hash_to_QC[instance_id][QC.parent_hash];
        else{
            exist = false;
        }
        hash_QC_lock[instance_id].unlock();
        if(!exist)
            break;
    }
    
    hash_QC_lock[instance_id].lock();
    string hash = t_man->get_hash();
    hash_to_QC[instance_id].erase(hash);
    for(auto &p : t_man->hash_voters){
        hash_to_txnid[instance_id].erase(p.first);
    }
    txnid_to_hash[instance_id].erase(t_man->get_txn_id());
    hash_QC_lock[instance_id].unlock();  
    
    tmsg = Message::create_message(t_man, EXECUTE_MSG);
    work_queue.enqueue(get_thd_id(), tmsg, false);
}

void WorkerThread::update_lockQC(const QuorumCertificate& QC, uint64_t view, uint64_t txnid, uint64_t instance_id)
{    
    QuorumCertificate B1, B2, B3;
    B1 = QC;

    hash_QC_lock[instance_id].lock();
    B2 = hash_to_QC[instance_id][B1.parent_hash];
    B3 = hash_to_QC[instance_id][B2.parent_hash];
    hash_QC_lock[instance_id].unlock();

    if(B1.viewNumber + 1 == view){
        // cout << "[A]" << txnid << endl;
        set_g_preparedQC(B1, instance_id, txnid);
    }

    if(B1.viewNumber + 1 == view && B2.viewNumber + 2 == view){
        // cout << "[B]" << txnid << endl;
        set_g_lockedQC(B2, instance_id, txnid - get_totInstances() * get_batch_size());
    }
    
    if(B1.viewNumber + 1 == view && B2.viewNumber + 2 == view && B3.viewNumber + 3 == view){
        // cout << "[C]" << txnid << endl;
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][B3.batch_hash].type = COMMIT;
        hash_QC_lock[instance_id].unlock();
        TxnManager *t_man = get_transaction_manager(txnid - 2 * get_totInstances()* get_batch_size(), 0);
        send_execute_msg_hotstuff(t_man, instance_id);
    }
}
#endif