#include "txn.h"
#include "wl.h"
#include "query.h"
#include "thread.h"
#include "mem_alloc.h"
#include "msg_queue.h"
#include "pool.h"
#include "message.h"
#include "ycsb_query.h"
#include "array.h"
#include "work_queue.h"

void TxnStats::init()
{
    starttime = 0;
    wait_starttime = get_sys_clock();
    total_process_time = 0;
    process_time = 0;
    total_local_wait_time = 0;
    local_wait_time = 0;
    total_remote_wait_time = 0;
    remote_wait_time = 0;
    write_cnt = 0;
    abort_cnt = 0;

    total_work_queue_time = 0;
    work_queue_time = 0;
    total_work_queue_cnt = 0;
    work_queue_cnt = 0;
    total_msg_queue_time = 0;
    msg_queue_time = 0;
    total_abort_time = 0;
    time_start_pre_prepare = 0;
    time_start_prepare = 0;
    time_start_commit = 0;
    time_start_execute = 0;

    clear_short();
}

void TxnStats::clear_short()
{
    work_queue_time_short = 0;
    cc_block_time_short = 0;
    cc_time_short = 0;
    msg_queue_time_short = 0;
    process_time_short = 0;
    network_time_short = 0;
}

void TxnStats::reset()
{
    wait_starttime = get_sys_clock();
    total_process_time += process_time;
    process_time = 0;
    total_local_wait_time += local_wait_time;
    local_wait_time = 0;
    total_remote_wait_time += remote_wait_time;
    remote_wait_time = 0;
    write_cnt = 0;

    total_work_queue_time += work_queue_time;
    work_queue_time = 0;
    total_work_queue_cnt += work_queue_cnt;
    work_queue_cnt = 0;
    total_msg_queue_time += msg_queue_time;
    msg_queue_time = 0;

    clear_short();
}

void TxnStats::abort_stats(uint64_t thd_id)
{
    total_process_time += process_time;
    total_local_wait_time += local_wait_time;
    total_remote_wait_time += remote_wait_time;
    total_work_queue_time += work_queue_time;
    total_msg_queue_time += msg_queue_time;
    total_work_queue_cnt += work_queue_cnt;
    assert(total_process_time >= process_time);
}

void TxnStats::commit_stats(uint64_t thd_id, uint64_t txn_id, uint64_t batch_id, uint64_t timespan_long,
                            uint64_t timespan_short)
{
    total_process_time += process_time;
    total_local_wait_time += local_wait_time;
    total_remote_wait_time += remote_wait_time;
    total_work_queue_time += work_queue_time;
    total_msg_queue_time += msg_queue_time;
    total_work_queue_cnt += work_queue_cnt;
    assert(total_process_time >= process_time);

    if (IS_LOCAL(txn_id))
    {
        PRINT_LATENCY("lat_s %ld %ld %f %f %f %f\n", txn_id, work_queue_cnt, (double)timespan_short / BILLION, (double)work_queue_time / BILLION, (double)msg_queue_time / BILLION, (double)process_time / BILLION);
    }
    else
    {
        PRINT_LATENCY("lat_rs %ld %ld %f %f %f %f\n", txn_id, work_queue_cnt, (double)timespan_short / BILLION, (double)total_work_queue_time / BILLION, (double)total_msg_queue_time / BILLION, (double)total_process_time / BILLION);
    }

    if (!IS_LOCAL(txn_id))
    {
        return;
    }
}

void Transaction::init()
{
    txn_id = UINT64_MAX;
    batch_id = UINT64_MAX;

    reset(0);
}

void Transaction::reset(uint64_t pool_id)
{
    rc = RCOK;
}

void Transaction::release(uint64_t pool_id)
{
    DEBUG("Transaction release\n");
}

void TxnManager::init(uint64_t pool_id, Workload *h_wl)
{
    if (!txn)
    {
        DEBUG_M("Transaction alloc\n");
        txn_pool.get(pool_id, txn);
    }
#if !BANKING_SMART_CONTRACT
    if (!query)
    {
        DEBUG_M("TxnManager::init Query alloc\n");
        qry_pool.get(pool_id, query);
        // this->query->init();
    }
#endif
    sem_init(&rsp_mutex, 0, 1);
    return_id = UINT64_MAX;

    this->h_wl = h_wl;

    txn_ready = true;


    chkpt_cnt = 1;
    chkpt_flag = false;

    new_view_vote_cnt = 0;
    generic_received = false;
    proposal_received = false;

    batchreq = NULL;

    txn_stats.init();
}

// reset after abort
void TxnManager::reset()
{
    rsp_cnt = 0;
    aborted = false;
    return_id = UINT64_MAX;
    //twopl_wait_start = 0;

    assert(txn);
#if BANKING_SMART_CONTRACT
    //assert(smart_contract);
#else
    assert(query);
#endif
    txn->reset(get_thd_id());

    // Stats
    txn_stats.reset();
}

void TxnManager::release(uint64_t pool_id)
{

    uint64_t tid = get_txn_id();

#if BANKING_SMART_CONTRACT
    delete this->smart_contract;
#else
    qry_pool.put(pool_id, query);
    query = NULL;
#endif
    txn_pool.put(pool_id, txn);
    txn = NULL;

    txn_ready = true;

    hash.clear();

#if EQUIV_TEST
    hash2.clear();
#endif

    prepared = false;

    chkpt_cnt = 1;
    chkpt_flag = false;

    new_viewed = false;
    new_view_vote_cnt = 0;
    for(auto& v: hash_voters){
        v.second.clear();
    }
    hash_voters.clear();
    generic_received = false;
    proposal_received = false;

    sync_sent = false;

#if ENABLE_ASK
    waiting_ask_resp = false;
#endif

#if EQUIV_TEST
    executable = true;
#endif

    release_all_messages(tid);

}

void TxnManager::reset_query()
{
#if !BANKING_SMART_CONTRACT
    ((YCSBQuery *)query)->reset();
#endif
}

RC TxnManager::commit()
{
    DEBUG("Commit %ld\n", get_txn_id());

    commit_stats();
    return Commit;
}

RC TxnManager::start_commit()
{
    RC rc = RCOK;
    DEBUG("%ld start_commit RO?\n", get_txn_id());
    return rc;
}

int TxnManager::received_response(RC rc)
{
    assert(txn->rc == RCOK);
    if (txn->rc == RCOK)
        txn->rc = rc;

    --rsp_cnt;

    return rsp_cnt;
}

bool TxnManager::waiting_for_response()
{
    return rsp_cnt > 0;
}

void TxnManager::commit_stats()
{
    uint64_t commit_time = get_sys_clock();
    uint64_t timespan_short = commit_time - txn_stats.restart_starttime;
    uint64_t timespan_long = commit_time - txn_stats.starttime;
    INC_STATS(get_thd_id(), total_txn_commit_cnt, 1);

    if (!IS_LOCAL(get_txn_id()))
    {
        txn_stats.commit_stats(get_thd_id(), get_txn_id(), get_batch_id(), timespan_long, timespan_short);
        return;
    }
    
    INC_STATS(get_thd_id(), txn_run_time, timespan_long);
    INC_STATS(get_thd_id(), single_part_txn_cnt, 1);
    txn_stats.commit_stats(get_thd_id(), get_txn_id(), get_batch_id(), timespan_long, timespan_short);
}

void TxnManager::register_thread(Thread *h_thd)
{
    this->h_thd = h_thd;
}

void TxnManager::set_txn_id(txnid_t txn_id)
{
    txn->txn_id = txn_id;
}

txnid_t TxnManager::get_txn_id()
{
    return txn->txn_id;
}

Workload *TxnManager::get_wl()
{
    return h_wl;
}

uint64_t TxnManager::get_thd_id()
{
    if (h_thd)
        return h_thd->get_thd_id();
    else
        return 0;
}

BaseQuery *TxnManager::get_query()
{
#if !BANKING_SMART_CONTRACT
    return query;
#else
    return NULL;
#endif
}

void TxnManager::set_query(BaseQuery *qry)
{
#if !BANKING_SMART_CONTRACT
    query = qry;
#endif
}

uint64_t TxnManager::incr_rsp(int i)
{
    uint64_t result;
    sem_wait(&rsp_mutex);
    result = ++this->rsp_cnt;
    sem_post(&rsp_mutex);
    return result;
}

uint64_t TxnManager::decr_rsp(int i)
{
    uint64_t result;
    sem_wait(&rsp_mutex);
    result = --this->rsp_cnt;
    sem_post(&rsp_mutex);
    return result;
}

RC TxnManager::validate()
{
    return RCOK;
}

/* Generic Helper functions. */

string TxnManager::get_hash()
{
    return hash;
}

void TxnManager::set_hash(string hsh)
{
    hash = hsh;
    hashSize = hash.length();
}

uint64_t TxnManager::get_hashSize()
{
    return hashSize;
}

void TxnManager::set_primarybatch(BatchRequests *breq)
{
    char *buf = create_msg_buffer(breq);
    Message *deepMsg = deep_copy_msg(buf, breq);
    batchreq = (BatchRequests *)deepMsg;
    delete_msg_buffer(buf);
}

bool TxnManager::is_chkpt_ready()
{
    return chkpt_flag;
}

void TxnManager::set_chkpt_ready()
{
    chkpt_flag = true;
}

uint64_t TxnManager::decr_chkpt_cnt()
{
    chkpt_cnt--;
    return chkpt_cnt;
}

uint64_t TxnManager::get_chkpt_cnt()
{
    return chkpt_cnt;
}

/************************************/

void TxnManager::release_all_messages(uint64_t txn_id)
{
    if ((txn_id + 1) % get_batch_size() == 0)
    {

#if CONSENSUS == HOTSTUFF
        vote_new_view.clear(); 
    #if THRESHOLD_SIGNATURE
        preparedQC.signature_share_map.clear();
        precommittedQC.signature_share_map.clear();
        committedQC.signature_share_map.clear();
        genericQC.release();
        highQC.release();
    #endif
        if(propmsg){
            Message::release_message(propmsg, 6);
        }
#else
        // Message::release_message(batchreq);
        if(batchreq){
             Message::release_message(batchreq);
         }
#endif
    }
}

//broadcasts checkpoint message to all nodes
void TxnManager::send_checkpoint_msgs()
{
    DEBUG("%ld Send PBFT_CHKPT_MSG message to %d\n nodes", get_txn_id(), g_node_cnt - 1);
    Message *msg = Message::create_message(this, PBFT_CHKPT_MSG);
    CheckpointMessage *ckmsg = (CheckpointMessage *)msg;
    // vector<uint64_t> dest;
    // for (uint64_t i = 0; i < g_node_cnt; i++)
    // {
    //     if (i == g_node_id)
    //     {
    //         continue;
    //     }
    //     dest.push_back(i);
    // }

    // msg_queue.enqueue(get_thd_id(), ckmsg, dest);
    // dest.clear();
    work_queue.enqueue(get_thd_id(), ckmsg, false);
}


#if CONSENSUS == HOTSTUFF
bool TxnManager::is_new_viewed(){
    return new_viewed;
}

void TxnManager::set_new_viewed(){
    new_viewed = true;
}

#if SEPARATE
void TxnManager::set_primarybatch(SpotLessProposalMsg *prop){
    char *buf = create_msg_buffer(prop);
    Message *deepMsg = deep_copy_msg(buf, prop); 
    propmsg = (SpotLessProposalMsg*)deepMsg;
    delete_msg_buffer(buf);
}
#endif

bool TxnManager::send_hotstuff_newview(SpotLessSyncMsg* nmsg){
    uint64_t view = get_current_view(instance_id);
    if(nmsg){
        view = nmsg->view;
    }
#if IGNORE_TEST
    if(g_node_id % IGNORE_FREQ == IGNORE_ID && g_node_id / IGNORE_FREQ < IGNORE_CNT&& g_node_id != get_view_primary(view, instance_id)){
        return false;
    }
#endif
    if(this->sync_sent){
        return false;
    }
    this->sync_sent = true;

    uint64_t dest_node_id = get_view_primary(view + 1, instance_id);
    #if PROCESS_PRINT
    printf("%ld Send SpotLess_SYNC_MSG message to %ld   %f\n", get_txn_id(), dest_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
    #endif

    vector<uint64_t> dest;
    if(nmsg)
        this->set_hash(nmsg->hash);

    Message *msg = Message::create_message(this, SpotLess_SYNC_MSG);
    Message *msg2 = Message::create_message(this, SpotLess_SYNC_MSG);
    
    SpotLessSyncMsg *nvmsg = (SpotLessSyncMsg *)msg;
    SpotLessSyncMsg *nvmsg2 = (SpotLessSyncMsg *)msg2;

    if(nmsg){
        nvmsg->hash = nvmsg2->hash = nmsg->hash;
        nvmsg->highQC = nvmsg2->highQC = nmsg->highQC;
        nvmsg->hashSize = nvmsg2->hashSize = nmsg->hashSize;
        nvmsg->view = nvmsg2->view = view;
    }

    nvmsg->digital_sign();
    nvmsg->sig_empty = false;

    if(g_node_id != dest_node_id)
    {
        if(nvmsg->instance_id == 0){
            nvmsg->force = true;
        }
        dest.push_back(dest_node_id);   //send new view to the next primary 
        msg_queue.enqueue(get_thd_id(), nvmsg, dest);
        dest.clear();
    }else{
        // cout << "[CC]" << endl;
        // fflush(stdout);
        this->genericQC.signature_share_map[g_node_id] = nvmsg->sig_share;
    }

    for(uint64_t i = 0; i < g_node_cnt; i++){
        if(i==g_node_id || i==dest_node_id){
            continue;
        }
        // cout << "[O]" << i << endl;
        dest.push_back(i);
    }
    if(nvmsg2->instance_id == 0){
        nvmsg2->force = true;
    }

#if DARK_TEST
    if(g_node_id % DARK_FREQ == DARK_ID && g_node_id / DARK_FREQ < DARK_CNT){
        dest.clear();
        for(uint64_t i=0; i<g_node_cnt; i++){
            if((i==g_node_id || i==dest_node_id || i % DARK_FREQ == VICTIM_ID)){
                continue;
            }
            dest.push_back(i);
        }
    }
#endif

    msg_queue.enqueue(get_thd_id(), nvmsg2, dest);
    dest.clear();

#if SEPARATE
    hash_voters[this->hash].push_back(g_node_id);
    uint64_t val = hash_voters[this->hash].size();
    if(val > this->new_view_vote_cnt){
        this->new_view_vote_cnt = val;
    }
    if(this->new_view_vote_cnt == nf){
        this->set_new_viewed();
    }
#endif

    return true;
}


#if SEPARATE
void TxnManager::send_hotstuff_generic(){
    Message *msg = Message::create_message(this, SpotLess_GENERIC_MSG);
    SpotLessGenericMsg *gene = (SpotLessGenericMsg *)msg;
    assert(!this->get_hash().empty());
    this->highQC = gene->highQC = get_g_preparedQC(instance_id);

    #if MAC_VERSION
    gene->digital_sign();
    this->psig_share = gene->psig_share;
    #endif

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

    msg_queue.enqueue(get_thd_id(), gene, dest);
    dest.clear();
}
#endif

#if EQUIV_TEST
void TxnManager::equivocate_generic(){
    Message *msg = Message::create_message(this, SpotLess_GENERIC_MSG);
    SpotLessGenericMsg *gene = (SpotLessGenericMsg *)msg;
    Message *msg2 = Message::create_message(this, SpotLess_GENERIC_MSG);
    SpotLessGenericMsg *gene2 = (SpotLessGenericMsg *)msg2;
    // to other malicious replicas
    Message *msg3 = Message::create_message(this, SpotLess_GENERIC_MSG);
    SpotLessGenericMsg *gene3 = (SpotLessGenericMsg *)msg3;
    assert(!this->get_hash().empty());
    this->highQC = gene->highQC = gene2->highQC = gene3->highQC = get_g_preparedQC(instance_id);
    
    gene2->hash = this->hash2;
    gene2->hashSize = this->hashSize2;
    
    gene3->hash = (this->hash+this->hash2).substr(0, this->hashSize + this->hashSize2);
    gene3->hashSize = gene3->hash.size();

    // cout << "[A]" << gene->hash << endl;
    // cout << "[B]" << gene2->hash << endl;
    // cout << "[C]" << gene3->hash << endl;
    gene->digital_sign();
    // cout << "[A]" << endl;
    gene2->digital_sign();
    // cout << "[B]" << endl;
    gene3->digital_sign();
    // cout << "[C]" << endl;
    this->psig_share = gene->psig_share;

    cout << gene3->hashSize << endl;
    assert(gene3->hashSize == 64);

    vector<uint64_t> dest;
    vector<uint64_t> dest2;
    vector<uint64_t> dest3;

    for(uint64_t i = 0; i < g_node_cnt; i++){
        if(i==g_node_id){
            continue;
        }else if(i % EQUIV_FREQ == EQ_VICTIM_ID){
            dest2.push_back(i);
        }else if(i % EQUIV_FREQ == EQUIV_ID && i / EQUIV_FREQ < EQUIV_CNT) {
            dest3.push_back(i);
        }else{
            dest.push_back(i);
        }
    }

    if(!dest.empty())
        msg_queue.enqueue(get_thd_id(), gene, dest);
    if(!dest2.empty())
        msg_queue.enqueue(get_thd_id(), gene2, dest2);
    if(!dest3.empty())
        msg_queue.enqueue(get_thd_id(), gene3, dest3);
    dest.clear();
    dest2.clear();
    dest3.clear();
    // cout << "[D]" << endl;
    fflush(stdout);
}

bool TxnManager::equivocate_hotstuff_newview(bool is_equi){

    uint64_t dest_node_id = get_view_primary(get_current_view(instance_id) + 1, instance_id);
    #if PROCESS_PRINT
    printf("%ld Send SpotLess_SYNC_MSG message to %ld   %f\n", get_txn_id(), dest_node_id, simulation->seconds_from_start(get_sys_clock()));
    fflush(stdout);
    #endif
    vector<uint64_t> dest;

    Message *msg = Message::create_message(this, SpotLess_SYNC_MSG);
    Message *msg2 = Message::create_message(this, SpotLess_SYNC_MSG);
    
    SpotLessSyncMsg *nvmsg = (SpotLessSyncMsg *)msg;
    SpotLessSyncMsg *nvmsg2 = (SpotLessSyncMsg *)msg2;

    if(is_equi){
        // assert(32 == hashSize2);
        // assert(hash2.size() == hashSize2);
        nvmsg->hash = nvmsg2->hash = this->hash2;
        nvmsg->hashSize = nvmsg2->hashSize = this->hashSize2;
    }

    nvmsg->digital_sign();
    nvmsg->sig_empty = false;

    if(instance_id == 0){
        nvmsg->force = true;
        nvmsg2->force = true;
    }

    if(g_node_id != dest_node_id)
    {
        if(is_equi && dest_node_id % EQUIV_FREQ == EQ_VICTIM_ID || 
        !is_equi && dest_node_id % EQUIV_FREQ != EQ_VICTIM_ID){
            #if SEND_NEWVIEW_PRINT
                printf("%ld Send SpotLess_SYNC_MSG message to %ld   %f\n", get_txn_id(), dest_node_id, simulation->seconds_from_start(get_sys_clock()));
                fflush(stdout);
            #endif
            dest.push_back(dest_node_id);   //send new view to the next primary 
            msg_queue.enqueue(get_thd_id(), nvmsg, dest);
            dest.clear();
        }
    }else if(!is_equi){
        this->genericQC.signature_share_map[g_node_id] = nvmsg->sig_share;
    }

    for(uint64_t i = 0; i < g_node_cnt; i++){
        if(i==g_node_id || i==dest_node_id){
            continue;
        }
        if(is_equi && i % EQUIV_FREQ == EQ_VICTIM_ID || !is_equi && i % EQUIV_FREQ != EQ_VICTIM_ID)
            dest.push_back(i);
    }

    msg_queue.enqueue(get_thd_id(), nvmsg2, dest);
    dest.clear();

#if SEPARATE
    if(!is_equi){
        this->hash_voters[this->hash].push_back(g_node_id);
        uint64_t val = this->hash_voters[this->hash].size();
        if(val > this->new_view_vote_cnt){
            this->new_view_vote_cnt = val;
        }
        if(this->new_view_vote_cnt == nf){
            this->set_new_viewed();
        }
    }
#endif

    return true;
}

#endif

void TxnManager::send_spotless_ask(SpotLessSyncMsg *nvmsg){
#if PROCESS_PRINT
    printf("[SEND ASK]TID: %lu THD: %lu \n", this->get_txn_id(), get_thd_id());
#endif
    Message *msg = Message::create_message(this, SpotLess_ASK_MSG);
    SpotLessAskMsg *ask = (SpotLessAskMsg *)msg;
    ask->hash = nvmsg->hash;
    ask->hashSize = nvmsg->hashSize;
    this->highQC = nvmsg->highQC;
    ask->view = nvmsg->view;
    ask->instance_id = nvmsg->instance_id;
    vector<uint64_t> dest;
#if DARK_TEST
    dest.push_back(g_node_id - 1);
#else
    for(uint i=1; i<vote_new_view.size(); i++){
        dest.push_back(vote_new_view[i]);
    } 
#endif
    msg_queue.enqueue(get_thd_id(), ask, dest);
    dest.clear();
}

#endif