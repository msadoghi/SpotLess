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
    case BATCH_REQ:
        rc = process_batch(msg);
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
#if VIEW_CHANGES
    case VIEW_CHANGE:
        rc = process_view_change_msg(msg);
        break;
    case NEW_VIEW:
        rc = process_new_view_msg(msg);
        break;
#endif
    case PBFT_PREP_MSG:
        rc = process_pbft_prep_msg(msg);
        break;
    case PBFT_COMMIT_MSG:
        rc = process_pbft_commit_msg(msg);
        break;
#if SHARPER
    case SUPER_PROPOSE:
        if (is_in_same_shard(msg->return_node_id, g_node_id))
            rc = process_batch(msg);
        else
            rc = process_super_propose(msg);
        break;
#endif
#if RING_BFT
    case COMMIT_CERT_MSG:
        rc = process_commit_certificate(msg);
        break;
    case RING_PRE_PREPARE:
        rc = process_ringbft_preprepare(msg);
        break;
    case RING_COMMIT:
        rc = process_ringbft_commit(msg);
        break;
#endif
#if CONSENSUS == HOTSTUFF
    case HOTSTUFF_PREP_MSG:
        rc = process_hotstuff_prepare(msg);
        break;
    case HOTSTUFF_PREP_VOTE_MSG:
        rc = process_hotstuff_prepare_vote(msg);
        break;
    case HOTSTUFF_PRECOMMIT_MSG:
        rc = process_hotstuff_precommit(msg);
        break;
    case HOTSTUFF_PRECOMMIT_VOTE_MSG:
        rc = process_hotstuff_precommit_vote(msg);
        break;
    case HOTSTUFF_COMMIT_MSG:
        rc = process_hotstuff_commit(msg);
        break;
    case HOTSTUFF_COMMIT_VOTE_MSG:
        rc = process_hotstuff_commit_vote(msg);
        break;
    case HOTSTUFF_DECIDE_MSG:
        rc = process_hotstuff_decide(msg);
        break;
    case HOTSTUFF_NEW_VIEW_MSG:
        rc = process_hotstuff_new_view(msg);
        break;
#if CHAINED
    case HOTSTUFF_GENERIC_MSG:
        rc = process_hotstuff_generic(msg);
        break;
#endif

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

#if LOCAL_FAULT
        // Fail some node.
        for (uint64_t j = 1; j <= num_nodes_to_fail; j++)
        {
            uint64_t fnode = g_min_invalid_nodes + j;
            for (uint i = 0; i < g_send_thread_cnt; i++)
            {
                stopMTX[i].lock();
                stop_nodes[i].push_back(fnode);
                stopMTX[i].unlock();
            }
        }

        fail_nonprimary();
#endif
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

#if LOCAL_FAULT
/* This function focibly fails non-primary replicas. */
void WorkerThread::fail_nonprimary()
{
    if (g_node_id > g_min_invalid_nodes)
    {
        if (g_node_id - num_nodes_to_fail <= g_min_invalid_nodes)
        {
            uint64_t count = 0;
            while (true)
            {
                count++;
                if (count > 100000000)
                {
                    cout << "FAILING!!!";
                    fflush(stdout);
                    assert(0);
                }
            }
        }
    }
}

#endif

#if TIMER_ON
void WorkerThread::add_timer(Message *msg, string qryhash)
{
    // TODO if condition is for experimental purpose: force one view change
    //if (this->has_view_changed())
    //    return;
    
    char *tbuf = create_msg_buffer(msg);
    Message *deepMsg = deep_copy_msg(tbuf, msg);
    deepMsg->return_node_id = msg->return_node_id;
#if !SpotLess
    server_timer->startTimer(qryhash, deepMsg);
#endif
    delete_msg_buffer(tbuf);
}

void WorkerThread::remove_timer(string qryhash)
{
    // TODO if condition is for experimental purpose: force one view change
    //if (this->has_view_changed())
    //    return;
#if !SpotLess
    server_timer->endTimer(qryhash);
#endif
}

#if SpotLess
void WorkerThread::add_timer(Message *msg, string qryhash, uint64_t instance_id)
{
    // TODO if condition is for experimental purpose: force one view change
    //if (this->has_view_changed())
    //    return;
    
    char *tbuf = create_msg_buffer(msg);
    Message *deepMsg = deep_copy_msg(tbuf, msg);
    deepMsg->return_node_id = msg->return_node_id;
    server_timer[instance_id]->startTimer(qryhash, deepMsg);
    delete_msg_buffer(tbuf);
}

void WorkerThread::remove_timer(string qryhash, uint64_t instance_id)
{
    // TODO if condition is for experimental purpose: force one view change
    //if (this->has_view_changed())
    //    return;

    server_timer[instance_id]->endTimer(qryhash);
}
#endif

#endif


#if SpotLess_RECOVERY
/*
Each non-primary replica continuously checks the timer for each batch. 
If there is a timeout then it initiates the view change. 
This requires sending a view change message to each replica.
*/
#if !SpotLess
void WorkerThread::check_for_timeout()
{
    Timer *ptimer = NULL;
    if(g_node_id != get_view_primary(get_current_view(0)) && server_timer->checkTimer(ptimer))
    {
        // server_timer->pauseTimer();
        // NewView Interrupt
        TxnManager *tman = NULL;
            
        if(ptimer){
            // tman = get_transaction_manager(ptimer->get_msg()->txn_id + 2, 0);
            remove_timer(ptimer->get_hash());
        }
        hash_QC_lock.lock();
        #if !CHAINED
            uint64_t txn_id = hash_to_txnid[get_g_preparedQC().batch_hash];
        #else
            uint64_t txn_id = hash_to_txnid[get_g_preparedQC().batch_hash] + get_batch_size(); 
        #endif
        hash_QC_lock.unlock();
        tman = get_transaction_manager(txn_id, 0);
        tman->register_thread(this);
        printf("timeout in Thread %ld txnid = %ld\n", get_thd_id(), txn_id);
#if !STOP_NODE_SET
        // Proceed to the next view
        for(uint64_t i=0; i<g_total_thread_cnt; i++){
            set_view(i, get_current_view(i) + 1);
        }
        tman->send_hotstuff_newview();
#else
        stop_lock.lock();
        stop_node_set.insert(get_current_view(get_thd_id()) % g_node_cnt);
        stop_lock.unlock();
        bool failednode = false;
        do{
            // Proceed to the next view
            for(uint64_t i=0; i<g_total_thread_cnt; i++){
                set_view(i, get_current_view(i) + 1);
            }
            tman->send_hotstuff_newview(failednode);
        }while(failednode);
#endif
    }
}

#else

void WorkerThread::check_for_timeout()
{
    uint64_t instance_id = get_thd_id();
    while(instance_id < get_totInstances()){
        Timer *ptimer = NULL;
        if(g_node_id != get_view_primary(get_current_view(instance_id), instance_id) 
            && server_timer[instance_id]->checkTimer(ptimer))
        {
            // NewView Interrupt
            TxnManager *tman = NULL;
            if(ptimer){
                remove_timer(ptimer->get_hash(), instance_id);
            }
            hash_QC_lock[instance_id].lock();

            #if !CHAINED
            uint64_t txn_id = hash_to_txnid[instance_id][get_g_preparedQC(instance_id).batch_hash];
            #else
            uint64_t txn_id = hash_to_txnid[instance_id][get_g_preparedQC(instance_id).batch_hash] + get_totInstances() * get_batch_size(); 
            #endif

            hash_QC_lock[instance_id].unlock();
            tman = get_transaction_manager(txn_id, 0);
            tman->register_thread(this);
            printf("timeout in Thread %ld with txn %ld\n", instance_id, txn_id);
    #if !STOP_NODE_SET
            // Proceed to the next view
            set_view(instance_id, get_current_view(instance_id) + 1);
            tman->send_hotstuff_newview();
    #else
            stop_lock.lock();
            stop_node_set.insert(get_view_primary(get_current_view(instance_id), instance_id));
            stop_lock.unlock();
            bool failednode = false;
            do{
                // Proceed to the next view
                set_view(instance_id, get_current_view(instance_id) + 1);
                tman->send_hotstuff_newview(failednode);
            }while(failednode);
    #endif
        }
        instance_id += get_multi_threads();
    }
}

#endif


/* This function causes the forced failure of the primary replica at a 
desired time. */
void WorkerThread::fail_primary(uint64_t time, uint64_t instance_id)
{
    if (!simulation->is_warmup_done())
        return;
    uint64_t elapsesd_time = get_sys_clock() - simulation->warmup_end_time;
    if (g_node_id == 0 && elapsesd_time > time)
    {
        cout << "FAIL: " << instance_id << endl;
        fail_count++;
        if(fail_count == get_totInstances()){
            uint64_t count = 0;
            while(true){
                count++;
                if(count > 1000000000)
                    break;
            }
            assert(0);
        }
    }
}
#endif

#if VIEW_CHANGES 
/*
Each non-primary replica continuously checks the timer for each batch. 
If there is a timeout then it initiates the view change. 
This requires sending a view change message to each replica.
*/
void WorkerThread::check_for_timeout()
{
    // TODO if condition is for experimental purpose: force one view change
    if (this->has_view_changed())
        return;

    if (g_node_id != get_current_view(get_thd_id()) && server_timer->checkTimer())
    {
        // Pause the timer to avoid causing further view changes.
        server_timer->pauseTimer();

        // cout << "Begin Changing View" << endl;
        fflush(stdout);

        Message *msg = Message::create_message(VIEW_CHANGE);
        TxnManager *local_tman = get_transaction_manager(get_curr_chkpt(), 0);
        // cout << "Initializing" << endl;
        fflush(stdout);

        ((ViewChangeMsg *)msg)->init(get_thd_id(), local_tman);

        // cout << "Going to send" << endl;
        fflush(stdout);

        //send view change messages
        vector<uint64_t> dest;
        for (uint64_t i = 0; i < g_node_cnt; i++)
        {
            //avoid sending msg to old primary
            if (i == get_current_view(get_thd_id()))
            {
                continue;
            }
            else if (i == g_node_id)
            {
                continue;
            }
            dest.push_back(i);
        }

        char *buf = create_msg_buffer(msg);
        Message *deepCMsg = deep_copy_msg(buf, msg);
        // Send to other replicas.
        msg_queue.enqueue(get_thd_id(), deepCMsg, dest);
        dest.clear();

        // process a message for itself
        deepCMsg = deep_copy_msg(buf, msg);
        work_queue.enqueue(get_thd_id(), deepCMsg, false);

        delete_msg_buffer(buf);
        Message::release_message(msg); // Releasing the message.

        fflush(stdout);
    }
}


/* This function causes the forced failure of the primary replica at a 
desired time. */
void WorkerThread::fail_primary(Message *msg, uint64_t time)
{
    if (!simulation->is_warmup_done())
        return;
    uint64_t elapsesd_time = get_sys_clock() - simulation->warmup_end_time;
    if (g_node_id == 0 && elapsesd_time > time)
    // if (g_node_id == 0 && msg->txn_id > 9)
    {
        uint64_t count = 0;
        while (true)
        {
            count++;
            if (count > 1000000000)
            {
                assert(0);
            }
        }
    }
}

void WorkerThread::store_batch_msg(BatchRequests *breq)
{
    char *bbuf = create_msg_buffer(breq);
    Message *deepCMsg = deep_copy_msg(bbuf, breq);
    storeBatch((BatchRequests *)deepCMsg);
    delete_msg_buffer(bbuf);
}

/*
The client forwarded its request to a non-primary replica. 
This maybe a potential case for a malicious primary.
Hence store the request, start timer and forward it to the primary replica.
*/
void WorkerThread::client_query_check(ClientQueryBatch *clbtch)
{
    // TODO if condition is for experimental purpose: force one view change
    if (this->has_view_changed())
        return;
        
    // cout << "REQUEST: " << clbtch->return_node_id << "\n";
    // fflush(stdout);

    //start timer when client broadcasts an unexecuted message
    // Last request of the batch.
    YCSBClientQueryMessage *qry = clbtch->cqrySet[clbtch->batch_size - 1];
    add_timer(clbtch, calculateHash(qry->getString()));

    // Forward to the primary.
    vector<uint64_t> dest;
    dest.push_back(get_current_view(get_thd_id()));

    char *tbuf = create_msg_buffer(clbtch);
    Message *deepCMsg = deep_copy_msg(tbuf, clbtch);
    msg_queue.enqueue(get_thd_id(), deepCMsg, dest);
    dest.clear();
    delete_msg_buffer(tbuf);
}

/****************************************/
/* Functions for handling view changes. */
/****************************************/

RC WorkerThread::process_view_change_msg(Message *msg)
{
    cout << "PROCESS VIEW CHANGE from" << msg->return_node_id << "\n";
    fflush(stdout);

    ViewChangeMsg *vmsg = (ViewChangeMsg *)msg;

    // Ignore the old view messages or those delivered after view change.
    if (vmsg->view <= get_current_view(get_thd_id()))
    {
        return RCOK;
    }

    //assert view is correct
    assert(vmsg->view == ((get_current_view(get_thd_id()) + 1) % g_node_cnt));

    //cout << "validating view change message" << endl;
    //fflush(stdout);

    if (!vmsg->addAndValidate(get_thd_id()))
    {
        //cout << "waiting for more view change messages" << endl;
        return RCOK;
    }

    //cout << "executing view change message" << endl;
    //fflush(stdout);

    // Only new primary performs rest of the actions.
    if (g_node_id == vmsg->view)
    {
        //cout << "New primary rules!" << endl;
        //fflush(stdout);

        // Move to next view
        uint64_t total_thds = g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt;
        for (uint64_t i = 0; i < total_thds; i++)
        {
            set_view(i, vmsg->view);
        }

        Message *newViewMsg = Message::create_message(NEW_VIEW);
        NewViewMsg *nvmsg = (NewViewMsg *)newViewMsg;
        nvmsg->init(get_thd_id());

        cout << "New primary is ready to fire" << endl;
        fflush(stdout);

        // Adding older primary to list of failed nodes.
        for (uint i = 0; i < g_send_thread_cnt; i++)
        {
            stopMTX[i].lock();
            stop_nodes[i].push_back(g_node_id - 1);
            stopMTX[i].unlock();
        }

        //send new view messages
        vector<uint64_t> dest;
        for (uint64_t i = 0; i < g_node_cnt; i++)
        {
            if (i == g_node_id)
            {
                continue;
            }
            dest.push_back(i);
        }

        char *buf = create_msg_buffer(nvmsg);
        Message *deepCMsg = deep_copy_msg(buf, nvmsg);
        msg_queue.enqueue(get_thd_id(), deepCMsg, dest);
        dest.clear();

        delete_msg_buffer(buf);
        Message::release_message(newViewMsg);

        // Remove all the ViewChangeMsgs.
        clearAllVCMsg();

        // Setting up the next txn id.
        set_next_idx(curr_next_index() / get_batch_size());

        set_expectedExecuteCount(curr_next_index() + get_batch_size() - 2);
        cout << "expectedExeCount = " << expectedExecuteCount << endl;
        fflush(stdout);

        // Start the re-directed requests.
        Timer *tmap;
        Message *retrieved_msg;
        for (uint64_t i = 0; i < server_timer->timerSize(); i++)
        {
            tmap = server_timer->fetchPendingRequests(i);
            retrieved_msg = tmap->get_msg();
            //YCSBClientQueryMessage *yc = ((ClientQueryBatch *)msg)->cqrySet[0];

            //cout << "MSG: " << yc->return_node << " :: Key: " << yc->requests[0]->key << "\n";
            //fflush(stdout);

            char *buf = create_msg_buffer(retrieved_msg);
            Message *deepCMsg = deep_copy_msg(buf, retrieved_msg);
            deepCMsg->return_node_id = retrieved_msg->return_node_id;

            // Assigning an identifier to the batch.
            deepCMsg->txn_id = get_and_inc_next_idx();
            work_queue.enqueue(get_thd_id(), deepCMsg, false);
            delete_msg_buffer(buf);
        }

        // Clear the timer.
        server_timer->removeAllTimers();
    }

    return RCOK;
}

RC WorkerThread::process_new_view_msg(Message *msg)
{
    cout << "PROCESS NEW VIEW " << msg->txn_id << "\n";
    fflush(stdout);

    NewViewMsg *nvmsg = (NewViewMsg *)msg;
    if (!nvmsg->validate(get_thd_id()))
    {
        assert(0);
        return RCOK;
    }

    // Adding older primary to list of failed nodes.
    for (uint i = 0; i < g_send_thread_cnt; i++)
    {
        stopMTX[i].lock();
        stop_nodes[i].push_back(nvmsg->view - 1);
        stopMTX[i].unlock();
    }

    // Move to the next view.
    uint64_t total_thds = g_thread_cnt + g_rem_thread_cnt + g_send_thread_cnt;
    for (uint64_t i = 0; i < total_thds; i++)
    {
        set_view(i, nvmsg->view);
    }


    //cout << "new primary changed view" << endl;
    //fflush(stdout);

    // Remove all the ViewChangeMsgs.
    clearAllVCMsg();

    // Setting up the next txn id.
    set_next_idx((curr_next_index() + 1) % get_batch_size());

    set_expectedExecuteCount(curr_next_index() + get_batch_size() - 2);

    // Clear the timer entries.
    server_timer->removeAllTimers();

    // Restart the timer.
    server_timer->resumeTimer();

    return RCOK;
}

#endif // VIEW_CHANGES

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

    uint64_t agCount = 0, ready_starttime, idle_starttime = 0;

    // Setting batch (only relevant for batching threads).
    next_set = 0;

    while (!simulation->is_done())
    {
        // uint64_t thd_id = get_thd_id();
        txn_man = NULL;
        heartbeat();
        progress_stats();
#if VIEW_CHANGES
        // Thread 0 continously monitors the timer for each batch.
        if (get_thd_id() == 0)
        {
            check_for_timeout();
        }

#endif
#if SEMA_TEST
        uint64_t thd_id = get_thd_id();
        idle_starttime = get_sys_clock();

        // Wait until there is at least one msg in its corresponding queue
        sem_wait(&worker_queue_semaphore[thd_id]);
        
        // The batch_thread waits until the replica becomes primary of at least one instance 
        if(thd_id == g_thread_cnt-3)  {
            sem_wait(&new_txn_semaphore);
        } 
        // The execute_thread waits until the next msg to execute is enqueued
        else if (thd_id == g_thread_cnt-2)  sem_wait(&execute_semaphore);

#if AUTO_POST && SpotLess_RECOVERY
#if !SpotLess
        while(thd_id == 0 && is_auto_posted()){
            check_for_timeout();
            set_auto_posted(false);
            sem_wait(&worker_queue_semaphore[0]);
        }
#else
        while(thd_id >= 0 && thd_id < get_multi_threads() && is_auto_posted(thd_id)){
            check_for_timeout();
            set_auto_posted(false, thd_id);
            sem_wait(&worker_queue_semaphore[thd_id]);
        }
#endif
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
#if TEMP_QUEUE
        else{
            #if !SpotLess
            if(thd_id == 0){
            #else
            if(thd_id < get_multi_threads()){
            #endif
                if(work_queue.check_view(msg)){
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


#if SHARPER
        if (!is_in_same_shard(msg->return_node_id, g_node_id))
        {
            string hash;
            if (msg->rtype == PBFT_PREP_MSG)
            {
                PBFTPrepMessage *prep = (PBFTPrepMessage *)msg;
                hash = prep->hash;
            }
            if (msg->rtype == PBFT_COMMIT_MSG)
            {
                PBFTCommitMessage *commit = (PBFTCommitMessage *)msg;
                hash = commit->hash;
            }
            if (msg->rtype == PBFT_PREP_MSG || msg->rtype == PBFT_COMMIT_MSG)
            {
                if (!digest_directory.exists(hash))
                {
                    // cout << "not found " << msg->return_node_id << "\t" << msg->txn_id << endl;
                    // fflush(stdout);
                    work_queue.enqueue(get_thd_id(), msg, true);
                    continue;
                }
                else
                {
                    msg->txn_id = digest_directory.get(hash) * get_batch_size() + get_batch_size() - 1;
                }
            }
        }
        if (msg->rtype != SUPER_PROPOSE && exception_msg_handling(msg))
        {
            continue;
        }
#else
        // Remove redundant messages.
        if (exception_msg_handling(msg))
        {
            continue;
        }
#endif

#if SHARPER
        if (msg->rtype != BATCH_REQ && msg->rtype != SUPER_PROPOSE && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG)
#elif RING_BFT
        if (msg->rtype == RING_COMMIT)
        {
            RingBFTCommit *rcm = (RingBFTCommit *)msg;
            if (rcm_checklist.exists(rcm->hash))
            {
                Message::release_message(msg);
                continue;
            }
            uint64_t txid = 0;
            if (!digest_directory.check_and_get(rcm->hash, txid))
            {
                work_queue.enqueue(get_thd_id(), msg, true);
                continue;
            }
            rcm->txn_id = txid + 2;
        }
        if (msg->rtype == COMMIT_CERT_MSG)
        {
            CommitCertificateMessage *ccm = (CommitCertificateMessage *)msg;
            if (ccm_checklist.check_and_add(ccm->hash))
            {
                Message::release_message(msg);
                continue;
            }
        }
        // Based on the type of the message, we try acquiring the transaction manager.
        if (msg->rtype != BATCH_REQ && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG && msg->rtype != RING_PRE_PREPARE && msg->rtype != COMMIT_CERT_MSG)

#elif CONSENSUS == HOTSTUFF
    #if !CHAINED
        if (msg->rtype != HOTSTUFF_PREP_MSG && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG)
    #else
        if (msg->rtype != HOTSTUFF_GENERIC_MSG && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG)
    #endif
#else
        // Based on the type of the message, we try acquiring the transaction manager.
        if (msg->rtype != BATCH_REQ && msg->rtype != CL_BATCH && msg->rtype != EXECUTE_MSG)
#endif
        {
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
        if (txn_man)
        {
            bool ready = txn_man->set_ready();
            if (!ready)
            {
                cout << "FAIL: " << txn_man->get_txn_id() << " :: RT: " << msg->rtype << "\n";
                fflush(stdout);
                assert(ready);
            }
        }
        // delete message
        ready_starttime = get_sys_clock();
        Message::release_message(msg);
        INC_STATS(get_thd_id(), worker_release_msg_time, get_sys_clock() - ready_starttime);
    }
#if SEMA_TEST
    for(uint i=0; i<g_thread_cnt; i++){
        sem_post(&worker_queue_semaphore[i]);
    }
    sem_post(&new_txn_semaphore);
    sem_post(&execute_semaphore);
    for(uint i=0; i<g_this_send_thread_cnt; i++){
        sem_post(&output_semaphore[i]);
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
void WorkerThread::init_txn_man(YCSBClientQueryMessage *clqry)
{
    txn_man->client_id = clqry->return_node;
    txn_man->client_startts = clqry->client_startts;
    YCSBQuery *query = (YCSBQuery *)(txn_man->query);
    for (uint64_t i = 0; i < clqry->requests.size(); i++)
    {
        ycsb_request *req = (ycsb_request *)mem_allocator.alloc(sizeof(ycsb_request));
        req->key = clqry->requests[i]->key;
        req->value = clqry->requests[i]->value;
        query->requests.add(req);
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
#if SHARPER
    tmsg->is_cross_shard = txn_man->is_cross_shard;
#endif
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
#if !MULTI_ON
RC WorkerThread::process_execute_msg(Message *msg)
{
    #if PROCESS_PRINT
    if (msg->txn_id / get_batch_size() % 100 == 0)
    {
        cout << "EXECUTE " << msg->txn_id << " THREAD: " << get_thd_id() << "\n";
        fflush(stdout);
    }
    #endif
    uint64_t ctime = get_sys_clock();

    // This message uses txn man of index calling process_execute.
#if SHARPER
    bool is_cross_shard = msg->is_cross_shard;
#endif
#if RING_BFT

    TxnManager *test = get_transaction_manager(msg->txn_id + 1, 0);
    bool local_request = is_local_request(test);
    bool is_cross_shard = test->is_cross_shard;
    ClientResponseMessage *crsp = 0;
    if (local_request)
    {
        Message *rsp = Message::create_message(CL_RSP);
        crsp = (ClientResponseMessage *)rsp;
        crsp->is_cross_shard = is_cross_shard;
        crsp->init();
    }
#else
    Message *rsp = Message::create_message(CL_RSP);
    ClientResponseMessage *crsp = (ClientResponseMessage *)rsp;
    crsp->init();
#endif

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

#if RING_BFT
        if (local_request)
        {
            crsp->copy_from_txn(tman);
        }
#else
        crsp->copy_from_txn(tman);
#endif
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

#if RING_BFT
        if (local_request)
        {
            crsp->copy_from_txn(tman);
        }
#else
        crsp->copy_from_txn(tman);
#endif
#if RING_BFT || SHARPER
        if (is_cross_shard)
        {
            INC_STATS(get_thd_id(), cross_shard_txn_cnt, 1);
        }
        else
        {
            INC_STATS(get_thd_id(), txn_cnt, 1);
        }
#else
        INC_STATS(get_thd_id(), txn_cnt, 1);
#endif

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
#if RING_BFT
    if (local_request)
    {
        crsp->copy_from_txn(txn_man);
        vector<uint64_t> dest;
        assert(is_local_request(txn_man));
        dest.push_back(txn_man->client_id);
        msg_queue.enqueue(get_thd_id(), crsp, dest);
        dest.clear();
    }
#elif SHARPER
    if (get_shard_number(txn_man->client_id) == get_shard_number(g_node_id))
    {
        crsp->copy_from_txn(txn_man);
        vector<uint64_t> dest;
        dest.push_back(txn_man->client_id);
        msg_queue.enqueue(get_thd_id(), crsp, dest);
        dest.clear();
    }
    else
    {
        Message::release_message(crsp);
    }
#else
    crsp->copy_from_txn(txn_man);

    vector<uint64_t> dest;
    dest.push_back(txn_man->client_id);
    msg_queue.enqueue(get_thd_id(), crsp, dest);
    dest.clear();
#endif
#if RING_BFT || SHARPER
    if (is_cross_shard)
    {
        INC_STATS(get_thd_id(), cross_shard_txn_cnt, 1);
    }
    else
    {
        INC_STATS(get_thd_id(), txn_cnt, 1);
    }
#else
    INC_STATS(get_thd_id(), txn_cnt, 1);
#endif
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
#endif //!MULTI_ON
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
    CheckpointMessage *ckmsg = (CheckpointMessage *)msg;
    #if PROCESS_PRINT
    printf("CHKPOINT from %ld:    %ld\n", msg->return_node_id, msg->txn_id);
    fflush(stdout);
    #endif
    // Check if message is valid.
    validate_msg(ckmsg);

    // If this checkpoint was already accessed, then return.
    if (txn_man->is_chkpt_ready())
    {
        return RCOK;
    }
    else
    {
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

#if CONSENSUS == HOTSTUFF
#if !SpotLess
    uint64_t min_stable_new_viewed = get_curr_new_viewed();
#else
    uint64_t min_stable_new_viewed = get_curr_new_viewed(0);
    for(uint i=1; i<get_totInstances(); i++){
        min_stable_new_viewed = min_stable_new_viewed < get_curr_new_viewed(i) ? min_stable_new_viewed : get_curr_new_viewed(i);
    }
#endif
#endif


    if (curr_next_index() > get_curr_chkpt())
    {
#if CONSENSUS == HOTSTUFF
        if(get_curr_chkpt() > min_stable_new_viewed){
            if(min_stable_new_viewed > get_batch_size())
                del_range = min_stable_new_viewed - get_batch_size();
        }else{
            if(get_curr_chkpt() > get_batch_size())
                del_range = get_curr_chkpt() - get_batch_size();
        }
#else
        if(get_curr_chkpt() > get_batch_size()){
            del_range = get_curr_chkpt() - get_batch_size();
        }
#endif
    }
    else
    {
#if CONSENSUS == HOTSTUFF
        if(curr_next_index() > min_stable_new_viewed){
            if(min_stable_new_viewed > get_batch_size())
                del_range = min_stable_new_viewed - get_batch_size();
        }else{
            if(curr_next_index() > get_batch_size())
                del_range = curr_next_index() - get_batch_size();
        }
#else
        if (curr_next_index() > get_batch_size())
        {
            del_range = curr_next_index() - get_batch_size();
        }
#endif
    }
    
    #if PROCESS_PRINT
    printf("Chkpt: %ld :: LD: %ld :: Del: %ld   Thread:%ld\n",msg->get_txn_id(), get_last_deleted_txn(), del_range, get_thd_id());
    fflush(stdout);
    #endif
    // Release Txn Managers.
    for (uint64_t i = get_last_deleted_txn(); i < del_range; i++)
    {
        release_txn_man(i, 0);
        inc_last_deleted_txn();

#if ENABLE_CHAIN
        if ((i + 1) % get_batch_size() == 0)
        {
            BlockChain->remove_block(i);
        }
#endif
    }
    
#if VIEW_CHANGES
    removeBatch(del_range);
#endif
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
#if CONSENSUS == HOTSTUFF
        if (msg->rtype != PBFT_CHKPT_MSG && msg->rtype != HOTSTUFF_NEW_VIEW_MSG)
#else
        if (msg->rtype != PBFT_CHKPT_MSG)
#endif
        {
            if (msg->txn_id <= curr_next_index())
            {
                // cout << "exception msg->txn_id = " << msg->txn_id << endl;
                Message::release_message(msg);
                return true;
            }
        }
#if CONSENSUS == HOTSTUFF
        else if(msg->rtype == HOTSTUFF_NEW_VIEW_MSG){
            #if !SpotLess
            if (((HOTSTUFFNewViewMsg*)msg)->view < get_current_view(0) || (((HOTSTUFFNewViewMsg*)msg)->view == get_current_view(0) && msg->txn_id <= get_curr_new_viewed()))
            #else
            uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
            if (((HOTSTUFFNewViewMsg*)msg)->view < get_current_view(instance_id) ||
             (((HOTSTUFFNewViewMsg*)msg)->view == get_current_view(instance_id) 
             && msg->txn_id <= get_curr_new_viewed(instance_id)))
            #endif
            {
                Message::release_message(msg);
                return true;
            }
        } 
#endif
        else
        {
            if (msg->txn_id <= get_curr_chkpt())
            {
                Message::release_message(msg);
                return true;
            }
        }
    }

    return false;
}

/** UNUSED */
void WorkerThread::algorithm_specific_update(Message *msg, uint64_t idx)
{
    #if MULTI_ON
    //For PBFT, update the instance_id field while creating txn managers
	if(msg->rtype == BATCH_REQ) {
	  BatchRequests *bmsg = (BatchRequests *)msg;
	  txn_man->instance_id = bmsg->view;
	} else {
	  txn_man->instance_id = g_node_id;
	}
    #endif
    // Can update any field, if required for a different consensus protocol.
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

/**
 * This function is used by the primary replicas to create and set 
 * transaction managers for each transaction part of the ClientQueryBatch message sent 
 * by the client. Further, to ensure integrity a hash of the complete batch is 
 * generated, which is also used in future communication.
 *
 * @param msg Batch of transactions as a ClientQueryBatch message.
 * @param tid Identifier for the first transaction of the batch.
 */
void WorkerThread::create_and_send_batchreq(ClientQueryBatch *msg, uint64_t tid)
{
    // Creating a new BatchRequests Message.
    Message *bmsg = Message::create_message(BATCH_REQ);
    BatchRequests *breq = (BatchRequests *)bmsg;
    breq->init(get_thd_id());

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

        // Setting up data for BatchRequests Message.
        breq->copy_from_txn(txn_man, msg->cqrySet[i]);

        // Reset this txn manager.
        bool ready = txn_man->set_ready();
        assert(ready);
    }

    // Now we need to unset the txn_man again for the last txn of batch.
    unset_ready_txn(txn_man);

    // Generating the hash representing the whole batch in last txn man.
    txn_man->set_hash(calculateHash(batchStr));
    txn_man->hashSize = txn_man->hash.length();

    breq->copy_from_txn(txn_man);

    // Storing the BatchRequests message.
    txn_man->set_primarybatch(breq);
#if SHARPER
    vector<uint64_t> dest;
#elif RING_BFT
    if (msg->is_cross_shard)
    {
        // txn_id x99
        txn_man->is_cross_shard = true;
        breq->is_cross_shard = true;
        digest_directory.add(breq->hash, breq->txn_id);
        for (uint64_t i = 0; i < g_shard_cnt; i++)
        {
            txn_man->involved_shards[i] = msg->involved_shards[i];
            breq->involved_shards[i] = msg->involved_shards[i];
        }
    }
    vector<uint64_t> dest;
#endif
    // Storing all the signatures.
    for (uint64_t i = 0; i < g_node_cnt; i++)
    {
#if RING_BFT || SHARPER
        if (i == g_node_id || !is_in_same_shard(i, g_node_id))
        {
            continue;
        }
        dest.push_back(i);
#else
        if (i == g_node_id)
        {
            continue;
        }
#endif
    }

    // Send the BatchRequests message to all the other replicas.
#if !RING_BFT && !SHARPER
    vector<uint64_t> dest = nodes_to_send(0, g_node_cnt);
#endif

    msg_queue.enqueue(get_thd_id(), breq, dest);
    dest.clear();
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
#if SHARPER
    case SUPER_PROPOSE:
#endif
    case BATCH_REQ:
        if (!((BatchRequests *)msg)->validate(get_thd_id()))
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
    case PBFT_PREP_MSG:
        if (!((PBFTPrepMessage *)msg)->validate())
        {
            assert(0);
        }
        break;
    case PBFT_COMMIT_MSG:
        if (!((PBFTCommitMessage *)msg)->validate())
        {
            assert(0);
        }
        break;

#if VIEW_CHANGES
    case VIEW_CHANGE:
        if (!((ViewChangeMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
    case NEW_VIEW:
        if (!((NewViewMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
#endif
#if CONSENSUS == HOTSTUFF && ENABLE_ENCRYPT
    case HOTSTUFF_PREP_MSG:
        if (!((HOTSTUFFPrepareMsg *)msg)->validate(get_thd_id()))
        {
            assert(0);
        }
        break;
    case HOTSTUFF_PREP_VOTE_MSG:
        if (!((HOTSTUFFPrepareVoteMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_PRECOMMIT_MSG:
        if (!((HOTSTUFFPreCommitMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_PRECOMMIT_VOTE_MSG:
        if (!((HOTSTUFFPreCommitVoteMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_COMMIT_MSG:
        if (!((HOTSTUFFCommitMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_COMMIT_VOTE_MSG:
        if (!((HOTSTUFFCommitVoteMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_DECIDE_MSG:
        if (!((HOTSTUFFDecideMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_NEW_VIEW_MSG:
        if (!((HOTSTUFFNewViewMsg *)msg)->validate())
        {
            assert(0);
        }
        break;
    case HOTSTUFF_GENERIC_MSG:
        if (!((HOTSTUFFGenericMsg *)msg)->validate(get_thd_id()))
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
#if CONSENSUS == HOTSTUFF
    #if !SpotLess
    uint64_t instance_id = 0;
    #else
    uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
    #endif
#endif
    if (msg->rtype == PBFT_PREP_MSG)
    {
        PBFTPrepMessage *pmsg = (PBFTPrepMessage *)msg;
        if(txn_man->get_hash().compare(pmsg->hash) == 0) { 
	        if((get_current_view(get_thd_id()) == pmsg->view)) {
	            return true;
            }
        } 
	}
    else if (msg->rtype == PBFT_COMMIT_MSG)
    {
        PBFTCommitMessage *cmsg = (PBFTCommitMessage *)msg;
        if ((txn_man->get_hash().compare(cmsg->hash) == 0) ||
            (get_current_view(get_thd_id()) == cmsg->view))
        {
            return true;
        }
    }
#if CONSENSUS == HOTSTUFF
    else if (msg->rtype == HOTSTUFF_PRECOMMIT_MSG){
        HOTSTUFFPreCommitMsg *pcmsg = (HOTSTUFFPreCommitMsg *)msg;
        if(txn_man->get_hash().compare(pcmsg->hash) == 0) { 
            if(get_current_view(instance_id) == pcmsg->view) {
                return true;
            }
        }
    }
    else if (msg->rtype == HOTSTUFF_COMMIT_MSG){
        HOTSTUFFCommitMsg *cmsg = (HOTSTUFFCommitMsg *)msg;
        if(txn_man->get_hash().compare(cmsg->hash) == 0) { 
            if(get_current_view(instance_id) == cmsg->view) {
                return true;
            }
        }
    }
    else if (msg->rtype == HOTSTUFF_DECIDE_MSG){
        HOTSTUFFDecideMsg *dmsg = (HOTSTUFFDecideMsg *)msg;
        if(txn_man->get_hash().compare(dmsg->hash) == 0) { 
            if(get_current_view(instance_id) == dmsg->view) {
                return true;
            }
        }
    }
    else if (msg->rtype == HOTSTUFF_PREP_VOTE_MSG){
        HOTSTUFFPrepareVoteMsg *pvmsg = (HOTSTUFFPrepareVoteMsg *)msg;
        if(txn_man->get_hash().compare(pvmsg->hash) == 0) { 
            if(get_current_view(instance_id) == pvmsg->view) {
                return true;
            }
        }
    }
    else if (msg->rtype == HOTSTUFF_PRECOMMIT_VOTE_MSG){
        HOTSTUFFPrepareVoteMsg *pvmsg = (HOTSTUFFPrepareVoteMsg *)msg;
        if(txn_man->get_hash().compare(pvmsg->hash) == 0) { 
            if(get_current_view(instance_id) == pvmsg->view) {
                return true;
            }
        }
    }
    else if (msg->rtype == HOTSTUFF_COMMIT_VOTE_MSG){
        HOTSTUFFCommitVoteMsg *cvmsg = (HOTSTUFFCommitVoteMsg *)msg;
        if(txn_man->get_hash().compare(cvmsg->hash) == 0) { 
            if(get_current_view(instance_id) == cvmsg->view) {
                return true;
            }
        }
    }
    else if (msg->rtype == HOTSTUFF_NEW_VIEW_MSG){
        HOTSTUFFNewViewMsg *nvmsg = (HOTSTUFFNewViewMsg *)msg;
        if(txn_man->get_hash().compare(nvmsg->hash) == 0) { 
            if(get_current_view(instance_id) == nvmsg->view) {
                return true;
            }
        }
    }
#endif
    return false;
}

/**
 * Checks if the incoming PBFTPrepMessage can be accepted.
 *
 * This functions checks if the hash and view of the commit message matches that of 
 * the Pre-Prepare message. Once 2f messages are received it returns a true and 
 * sets the `is_prepared` flag for furtue identification.
 *
 * @param msg PBFTPrepMessage.
 * @return bool True if the transactions of this batch are prepared.
 */
bool WorkerThread::prepared(PBFTPrepMessage *msg)
{
    //cout << "Inside PREPARED: " << txn_man->get_txn_id() << "\n";
    //fflush(stdout);

    // Once prepared is set, no processing for further messages.
    if (txn_man->is_prepared())
    {
        return false;
    }

    // If BatchRequests messages has not arrived yet, then return false.
    if (txn_man->get_hash().empty())
    {
        // Store the message.
        txn_man->info_prepare.push_back(msg->return_node);
        return false;
    }
    else
    {
        if (!checkMsg(msg))
        {
            // If message did not match.
            cout << txn_man->get_hash() << " :: " << msg->hash << "\n";
            cout << get_current_view(get_thd_id()) << " :: " << msg->view << "\n";
            fflush(stdout);
            return false;
        }
    }
#if SHARPER
    if (msg->is_cross_shard)
    {
        txn_man->decr_prep_rsp_cnt(get_shard_number(msg->return_node_id));
        for (uint64_t i = 0; i < g_shard_cnt; i++)
        {
            if (txn_man->prep_rsp_cnt_arr[i] != 0 && txn_man->involved_shards[i])
                return false;
        }
        txn_man->set_prepared();
        return true;
    }
#endif
    uint64_t prep_cnt = txn_man->decr_prep_rsp_cnt();
    if (prep_cnt == 0)
    {
        txn_man->set_prepared();
        return true;
    }

    return false;
}

#if CONSENSUS == HOTSTUFF
bool WorkerThread::hotstuff_prepared(HOTSTUFFPrepareVoteMsg* msg){
    // Once prepared is set, no processing for further messages.
    if (txn_man->is_prepared())
    {
        return false;
    }
    if(txn_man->get_hash().empty()){
        return false;
    }
    if (!checkMsg(msg))
    {
        #if !SpotLess
        uint64_t instance_id = get_thd_id();
        #else
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
        #endif
        // If message did not match.
        if(get_current_view(instance_id) < msg->view){
            cout << txn_man->get_hash() << " :: " << msg->hash << "\n";
            cout << get_current_view(instance_id) << " :: " << msg->view << "\n";
            fflush(stdout);
        }
        return false;
    }
    for(uint i=0; i<txn_man->vote_prepare.size(); i++){
        if(msg->return_node_id==txn_man->vote_prepare[i])
            return false;
    }
    txn_man->vote_prepare.push_back(msg->return_node_id);
#if THRESHOLD_SIGNATURE
    txn_man->preparedQC.signature_share_map[msg->return_node_id] = msg->sig_share;
#endif
    if (--txn_man->prepare_vote_cnt == 0)
    {
    #if !SpotLess
        txn_man->preparedQC.viewNumber =  get_g_preparedQC().genesis? 0:get_current_view(0);
        txn_man->preparedQC.parent_hash = get_g_preparedQC().batch_hash;
        txn_man->preparedQC.batch_hash = txn_man->get_hash();
        txn_man->preparedQC.type = PREPARE;
        hash_QC_lock.lock();
        hash_to_QC.insert(make_pair<string&,QuorumCertificate&>(txn_man->preparedQC.batch_hash, txn_man->preparedQC));
        set_g_preparedQC(txn_man->preparedQC);
        hash_to_txnid.insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, msg->txn_id));
        hash_QC_lock.unlock();
    #else
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
        txn_man->preparedQC.viewNumber = get_g_preparedQC(instance_id).genesis? 0:get_current_view(instance_id);
        txn_man->preparedQC.parent_hash = get_g_preparedQC(instance_id).batch_hash;
        txn_man->preparedQC.batch_hash = txn_man->get_hash();
        txn_man->preparedQC.type = PREPARE;

        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id].insert(make_pair<string&,QuorumCertificate&>(txn_man->preparedQC.batch_hash, txn_man->preparedQC));
        set_g_preparedQC(txn_man->preparedQC, instance_id);
        hash_to_txnid[instance_id].insert(make_pair<string&,uint64_t&>(txn_man->preparedQC.batch_hash, msg->txn_id));
        hash_QC_lock[instance_id].unlock();
    #endif
        txn_man->set_prepared();
        return true;
    }
    return false;
}

bool WorkerThread::hotstuff_precommitted(HOTSTUFFPreCommitVoteMsg* msg){
    if (txn_man->is_precommitted())
    {
        return false;
    }
    if(txn_man->get_hash().empty()){
        return false;
    }
    if (!checkMsg(msg))
    {
        #if !SpotLess
        uint64_t instance_id = get_thd_id();
        #else
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
        #endif
        // If message did not match.
        if(get_current_view(instance_id) < msg->view){
            cout << txn_man->get_hash() << " :: " << msg->hash << "\n";
            cout << get_current_view(instance_id) << " :: " << msg->view << "\n";
            fflush(stdout);
        }
        return false;
    }
    for(uint i=0; i<txn_man->vote_precommit.size(); i++){
        if(msg->return_node_id==txn_man->vote_precommit[i])
            return false;
    }
    txn_man->vote_precommit.push_back(msg->return_node_id);
#if THRESHOLD_SIGNATURE
    txn_man->precommittedQC.signature_share_map[msg->return_node_id] = msg->sig_share;
#endif
    if (--txn_man->precommit_vote_cnt == 0 && txn_man->is_prepared())
    {
        txn_man->precommittedQC.viewNumber = txn_man->preparedQC.viewNumber;
        txn_man->precommittedQC.parent_hash = txn_man->preparedQC.parent_hash;
        txn_man->precommittedQC.batch_hash = txn_man->preparedQC.batch_hash;
        txn_man->precommittedQC.type = PRECOMMIT;

#if !SpotLess
        hash_QC_lock.lock();
        hash_to_QC[txn_man->precommittedQC.batch_hash] = txn_man->precommittedQC;
        hash_QC_lock.unlock();
#else
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][txn_man->precommittedQC.batch_hash] = txn_man->precommittedQC;
        hash_QC_lock[instance_id].unlock();
#endif

        #if !SpotLess
        set_g_lockedQC(txn_man->precommittedQC);
        #else
        set_g_lockedQC(txn_man->precommittedQC, instance_id);
        #endif

        txn_man->set_precommitted();
    
        return true;
    }
    return false;
}

bool WorkerThread::hotstuff_committed(HOTSTUFFCommitVoteMsg* msg){
    if (txn_man->is_committed())
    {
        return false;
    }
    if(txn_man->get_hash().empty()){
        return false;
    }
    if (!checkMsg(msg))
    {
        #if !SpotLess
        uint64_t instance_id = get_thd_id();
        #else
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
        #endif
        // If message did not match.
        if(get_current_view(instance_id) < msg->view){
            cout << txn_man->get_hash() << " :: " << msg->hash << "\n";
            cout << get_current_view(instance_id) << " :: " << msg->view << "\n";
            fflush(stdout);
        }
        
        return false;
    }
    for(uint i=0; i<txn_man->vote_commit.size(); i++){
        if(msg->return_node_id==txn_man->vote_commit[i])
            return false;
    }
    txn_man->vote_commit.push_back(msg->return_node_id);
#if THRESHOLD_SIGNATURE
    txn_man->committedQC.signature_share_map[msg->return_node_id] = msg->sig_share;
#endif

    if (--txn_man->commit_vote_cnt == 0 && txn_man->is_precommitted())
    {
        txn_man->committedQC.viewNumber = txn_man->precommittedQC.viewNumber;
        txn_man->committedQC.parent_hash = txn_man->precommittedQC.parent_hash;
        txn_man->committedQC.batch_hash = txn_man->precommittedQC.batch_hash;
        txn_man->committedQC.type = COMMIT;

#if !SpotLess
        hash_QC_lock.lock();
        hash_to_QC[txn_man->committedQC.batch_hash] = txn_man->committedQC;
        hash_QC_lock.unlock();
#else
        uint64_t instance_id = msg->txn_id / get_batch_size() % get_totInstances();
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][txn_man->committedQC.batch_hash] = txn_man->committedQC;
        hash_QC_lock[instance_id].unlock();
#endif
        txn_man->set_committed();
        return true;
    }
    return false;
}

bool WorkerThread::hotstuff_new_viewed(HOTSTUFFNewViewMsg* msg){
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

    txn_man->vote_new_view.push_back(msg->return_node_id);
#if CHAINED || SHIFT_QC
    txn_man->genericQC.signature_share_map[msg->return_node_id] = msg->sig_share;
#endif

    if (--txn_man->new_view_vote_cnt == 0)
    {
        txn_man->set_new_viewed();
        return true;
    }
    return false;
}

/**
 * This function is used by the non-primary or backup replicas to create and set 
 * transaction managers for each transaction part of the HOTSTUFFPrepareMsg message sent by
 * the primary replica.
 *
 * @param prep Batch of transactions as a HOTSTUFFPrepareMsg message.
 * @param bid Another dimensional identifier to support more transaction managers.
 */
void WorkerThread::set_txn_man_fields(HOTSTUFFPrepareMsg *prep, uint64_t bid)
{
    for (uint64_t i = 0; i < get_batch_size(); i++)
    {
        txn_man = get_transaction_manager(prep->index[i], bid);
        unset_ready_txn(txn_man);
        txn_man->register_thread(this);
        txn_man->return_id = prep->return_node_id;
        // Fields that need to updated according to the specific algorithm.
        algorithm_specific_update(prep, i);
        init_txn_man(prep->requestMsg[i]);
        bool ready = txn_man->set_ready();
        assert(ready);
    }
    // We need to unset txn_man again for last txn in the batch.
    unset_ready_txn(txn_man);
    txn_man->set_hash(prep->hash);
}
#if !SpotLess
void WorkerThread::send_execute_msg_hotstuff()
#else
void WorkerThread::send_execute_msg_hotstuff(uint64_t instance_id)
#endif
{
    TxnManager* tman;
    Message *tmsg;
#if !SpotLess
    hash_QC_lock.lock();
    QuorumCertificate QC = hash_to_QC[txn_man->get_hash()];
    QC = hash_to_QC[QC.parent_hash];
    hash_QC_lock.unlock();
#else
    hash_QC_lock[instance_id].lock();
    QuorumCertificate QC = hash_to_QC[instance_id][txn_man->get_hash()];
    QC = hash_to_QC[instance_id][QC.parent_hash];
    hash_QC_lock[instance_id].unlock();
#endif
    while(QC.type != COMMIT && !QC.batch_hash.empty()){
#if !SpotLess
        hash_QC_lock.lock();
        tman = get_transaction_manager(hash_to_txnid[QC.batch_hash], 0);
        hash_QC_lock.unlock();
#else
        hash_QC_lock[instance_id].lock();
        tman = get_transaction_manager(hash_to_txnid[instance_id][QC.batch_hash], 0);
        hash_QC_lock[instance_id].unlock();
#endif
        tmsg = Message::create_message(tman, EXECUTE_MSG);
        work_queue.enqueue(get_thd_id(), tmsg, false);
        #if TIMER_ON
            // End the timer for this client batch.
            #if !SpotLess
                remove_timer(QC.batch_hash);
            #else
                remove_timer(QC.batch_hash, instance_id);
            #endif
        #endif
#if !SpotLess
        hash_QC_lock.lock();
        hash_to_QC[QC.batch_hash].type = COMMIT;
        QC = hash_to_QC[QC.parent_hash];
        hash_QC_lock.unlock();
    }
    hash_QC_lock.lock();
    hash_to_QC[txn_man->get_hash()].type = COMMIT;
    hash_QC_lock.unlock();
#else
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][QC.batch_hash].type = COMMIT;
        QC = hash_to_QC[instance_id][QC.parent_hash];
        hash_QC_lock[instance_id].unlock();
    }
    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][txn_man->get_hash()].type = COMMIT;
    hash_QC_lock[instance_id].unlock();  
#endif
    send_execute_msg();
}


#if !SpotLess
void WorkerThread::send_execute_msg_hotstuff(TxnManager *t_man)
#else
void WorkerThread::send_execute_msg_hotstuff(TxnManager *t_man, uint64_t instance_id)
#endif
{
    TxnManager* tman;
    Message *tmsg;
#if !SpotLess
    hash_QC_lock.lock();
    QuorumCertificate QC = hash_to_QC[t_man->get_hash()];
    QC = hash_to_QC[QC.parent_hash];
    hash_QC_lock.unlock();
#else
    hash_QC_lock[instance_id].lock();
    QuorumCertificate QC = hash_to_QC[instance_id][t_man->get_hash()];
    QC = hash_to_QC[instance_id][QC.parent_hash];
    hash_QC_lock[instance_id].unlock();
#endif
    
    while(QC.type != COMMIT && !QC.batch_hash.empty()){ 
#if !SpotLess
        hash_QC_lock.lock();
        tman = get_transaction_manager(hash_to_txnid[QC.batch_hash], 0);
        hash_QC_lock.unlock();
#else
        hash_QC_lock[instance_id].lock();
        tman = get_transaction_manager(hash_to_txnid[instance_id][QC.batch_hash], 0);
        hash_QC_lock[instance_id].unlock();
#endif
        tmsg = Message::create_message(tman, EXECUTE_MSG);
        work_queue.enqueue(get_thd_id(), tmsg, false);
        #if TIMER_ON
            // End the timer for this client batch.
            #if !SpotLess
                remove_timer(QC.batch_hash);
            #else
                remove_timer(QC.batch_hash, instance_id);
            #endif
        #endif
#if !SpotLess
        hash_QC_lock.lock();
        hash_to_QC[QC.batch_hash].type = COMMIT;
        QC = hash_to_QC[QC.parent_hash];
        hash_QC_lock.unlock();
    }
    hash_QC_lock.lock();
    hash_to_QC[t_man->get_hash()].type = COMMIT;
    hash_QC_lock.unlock();
#else
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][QC.batch_hash].type = COMMIT;
        QC = hash_to_QC[instance_id][QC.parent_hash];
        hash_QC_lock[instance_id].unlock();
    }
    
    hash_QC_lock[instance_id].lock();
    hash_to_QC[instance_id][t_man->get_hash()].type = COMMIT;
    hash_QC_lock[instance_id].unlock();  
    
#endif
    tmsg = Message::create_message(t_man, EXECUTE_MSG);
    
    work_queue.enqueue(get_thd_id(), tmsg, false);
}

#if CHAINED

#if !SpotLess
void WorkerThread::update_lockQC(const QuorumCertificate& QC, uint64_t view)
#else
void WorkerThread::update_lockQC(const QuorumCertificate& QC, uint64_t view, uint64_t instance_id)
#endif
{    
    QuorumCertificate B1, B2, B3;
    B1 = QC;

#if !SpotLess
    hash_QC_lock.lock();
    B2 = hash_to_QC[B1.parent_hash];
    B3 = hash_to_QC[B2.parent_hash];
    hash_QC_lock.unlock();
#else
    hash_QC_lock[instance_id].lock();
    B2 = hash_to_QC[instance_id][B1.parent_hash];
    B3 = hash_to_QC[instance_id][B2.parent_hash];
    hash_QC_lock[instance_id].unlock();
#endif

    if(B1.viewNumber + 1 == view){
        #if TIMER_ON
            #if !SpotLess
            remove_timer(B1.batch_hash);
            #else
            remove_timer(B1.batch_hash, instance_id);
            #endif
        #endif
        #if !SpotLess
        set_g_preparedQC(B1);
        #else
        set_g_preparedQC(B1, instance_id);
        #endif
    }
    if(B1.viewNumber + 1 == view && B2.viewNumber + 2 == view){
        #if !SpotLess
        hash_QC_lock.lock();
        hash_to_QC[B2.batch_hash].type = PRECOMMIT;
        set_g_lockedQC(B2);
        hash_QC_lock.unlock();
        #else
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][B2.batch_hash].type = PRECOMMIT;
        set_g_lockedQC(B2, instance_id);
        hash_QC_lock[instance_id].unlock();
        #endif
    }

    if(B1.viewNumber + 1 == view && B2.viewNumber + 2 == view && B3.viewNumber + 3 == view){
#if !SpotLess
        hash_QC_lock.lock();
        hash_to_QC[B3.batch_hash].type = COMMIT;
        uint64_t committed_txn_id = hash_to_txnid[B3.batch_hash];
        hash_QC_lock.unlock();
        TxnManager *t_man = get_transaction_manager(committed_txn_id, 0);
        send_execute_msg_hotstuff(t_man);
#else
        hash_QC_lock[instance_id].lock();
        hash_to_QC[instance_id][B3.batch_hash].type = COMMIT;
        uint64_t committed_txn_id = hash_to_txnid[instance_id][B3.batch_hash];
        hash_QC_lock[instance_id].unlock();
        TxnManager *t_man = get_transaction_manager(committed_txn_id, 0);
        send_execute_msg_hotstuff(t_man, instance_id);
#endif  
    }
}
    
#endif

#endif