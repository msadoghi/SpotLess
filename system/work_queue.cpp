#include "work_queue.h"
#include "mem_alloc.h"
#include "query.h"
#include "message.h"
#include "client_query.h"
#include <boost/lockfree/queue.hpp>

#if !(MULTI_ON||PVP)

QWorkQueue::~QWorkQueue()
{
    release();
}

void QWorkQueue::release()
{
    // Queue for worker thread 0.
    uint64_t effective_queue_cnt = 1;

#if EXECUTION_THREAD
    effective_queue_cnt += indexSize;
#endif

    // A queue for checkpoint messages.
    effective_queue_cnt++;

    if(work_queue){
        for(uint i=0; i<effective_queue_cnt; i++){
            delete work_queue[i];
            work_queue[i] = nullptr;
        }
        delete []work_queue;
        work_queue = nullptr;
    }
    if(new_txn_queue){
        delete new_txn_queue;
        new_txn_queue = nullptr;
    }

}

void QWorkQueue::init()
{

    // Queue for worker thread 0.
    uint64_t effective_queue_cnt = 1;

#if EXECUTION_THREAD
    effective_queue_cnt += indexSize;
#endif

    // A queue for checkpoint messages.
    effective_queue_cnt++;

#if RING_BFT
    effective_queue_cnt++;
#endif

    cout << "Total queues: " << effective_queue_cnt << "\n";
    fflush(stdout);

    work_queue = new boost::lockfree::queue<work_queue_entry *> *[effective_queue_cnt];
    for (uint64_t i = 0; i < effective_queue_cnt; i++)
    {
        work_queue[i] = new boost::lockfree::queue<work_queue_entry *>(0);
    }

    new_txn_queue = new boost::lockfree::queue<work_queue_entry *>(0);
#if TEMP_QUEUE
    temp_queue = new boost::lockfree::queue<work_queue_entry* > (0);
    new_view_queue = new boost::lockfree::queue<work_queue_entry* > (0);
#endif
}

#if ENABLE_PIPELINE
// If pipeline is enabled, then different worker_threads have different queues.
void QWorkQueue::enqueue(uint64_t thd_id, Message *msg, bool busy)
{
    uint64_t starttime = get_sys_clock();
    assert(msg);
    DEBUG_M("QWorkQueue::enqueue work_queue_entry alloc\n");
    work_queue_entry *entry = (work_queue_entry *)mem_allocator.align_alloc(sizeof(work_queue_entry));
    entry->msg = msg;
    entry->rtype = msg->rtype;
    entry->txn_id = msg->txn_id;
    entry->batch_id = msg->batch_id;
    entry->starttime = get_sys_clock();
    assert(ISSERVER || ISREPLICA);
    DEBUG("Work Enqueue (%ld,%ld) %d\n", entry->txn_id, entry->batch_id, entry->rtype);
#if RING_BFT
    if (msg->rtype == CL_QRY || msg->rtype == CL_BATCH || msg->rtype == COMMIT_CERT_MSG)
    {
        if (true)
#elif SHARPER
    if (msg->rtype == CL_QRY || msg->rtype == CL_BATCH)
    {
        if (g_node_id == view_to_primary(get_current_view(thd_id)))
#else
    if (msg->rtype == CL_QRY || msg->rtype == CL_BATCH)
    {
        if(true)
#endif
        {
            while (!new_txn_queue->push(entry) && !simulation->is_done())
            {
            }
            #if SEMA_TEST
            sem_post(&worker_queue_semaphore[g_thread_cnt - 3]);
            #endif
        }
        else
        {
            assert(entry->rtype < 100);
            while (!work_queue[0]->push(entry) && !simulation->is_done())
            {
            }
        }
    }

#if RING_BFT
    else if (msg->rtype == BATCH_REQ || msg->rtype == RING_PRE_PREPARE)
    {
        // Queue for Thread for ordered sending of batches.
        if (g_node_id != view_to_primary(get_current_view(thd_id)))
#else
    else if (msg->rtype == BATCH_REQ)
    {
        // Queue for Thread for ordered sending of batches.
        if (g_node_id != get_current_view(thd_id) % g_node_cnt)
#endif
        {
            while (!work_queue[0]->push(entry) && !simulation->is_done())
            {
            }
        }
        else
            assert(0);
    }
    else if (msg->rtype == EXECUTE_MSG)
    {
        // cout << "EE" << msg->txn_id << endl;
        fflush(stdout);
        uint64_t bid = ((msg->txn_id + 2) - get_batch_size()) / get_batch_size();
        uint64_t qid = bid % indexSize;
        while (!work_queue[qid + 1]->push(entry) && !simulation->is_done())
        {
        }

        #if SEMA_TEST
        if(msg->txn_id >= get_expectedExecuteCount()){
            sem_post(&worker_queue_semaphore[g_thread_cnt - 2]);
            execute_msg_heap_push(msg->txn_id);
            //if the next msg to execute is enqueued
            if(msg->txn_id == get_expectedExecuteCount()){
                sem_post(&execute_semaphore);
            }
        }
        #endif

    }
    else if (msg->rtype == PBFT_CHKPT_MSG)
    {
        while (!work_queue[indexSize + 1]->push(entry) && !simulation->is_done())
        {
        }
        #if SEMA_TEST
        sem_post(&worker_queue_semaphore[g_thread_cnt - 1]);
        #endif
    }
#if RING_BFT
    else if (msg->rtype == RING_COMMIT)
    {
        while (!work_queue[indexSize + 2]->push(entry) && !simulation->is_done())
        {
        }
    }
#endif
    else
    {
        assert(entry->rtype < 100);
        while (!work_queue[0]->push(entry) && !simulation->is_done())
        {
        }
        #if SEMA_TEST
        sem_post(&worker_queue_semaphore[0]);
        #endif
    }

    INC_STATS(thd_id, work_queue_enqueue_time, get_sys_clock() - starttime);
    INC_STATS(thd_id, work_queue_enq_cnt, 1);
}

Message *QWorkQueue::dequeue(uint64_t thd_id)
{
    uint64_t starttime = get_sys_clock();
    assert(ISSERVER || ISREPLICA);
    Message *msg = NULL;
    work_queue_entry *entry = NULL;

    bool valid = false;

    // Thread 0 only looks at work queue
    if (thd_id == 0)
    {
        valid = work_queue[0]->pop(entry);
#if CONSENSUS == HOTSTUFF
        if(valid)
        {
            uint64_t msg_view = 0;
            if(entry->msg->rtype == HOTSTUFF_PREP_MSG || entry->msg->rtype == HOTSTUFF_GENERIC_MSG){
                HOTSTUFFPrepareMsg *pmsg = (HOTSTUFFPrepareMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_PREP_VOTE_MSG){
                HOTSTUFFPrepareVoteMsg *pmsg = (HOTSTUFFPrepareVoteMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_PRECOMMIT_VOTE_MSG){
                HOTSTUFFPreCommitVoteMsg *pmsg = (HOTSTUFFPreCommitVoteMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_COMMIT_VOTE_MSG){
                HOTSTUFFCommitVoteMsg *pmsg = (HOTSTUFFCommitVoteMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_NEW_VIEW_MSG){
                HOTSTUFFNewViewMsg *pmsg = (HOTSTUFFNewViewMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_PRECOMMIT_MSG){
                HOTSTUFFPreCommitMsg *pmsg = (HOTSTUFFPreCommitMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_COMMIT_MSG){
                HOTSTUFFCommitMsg *pmsg = (HOTSTUFFCommitMsg*)(entry->msg);
                msg_view = pmsg->view;
            }else if(entry->msg->rtype == HOTSTUFF_DECIDE_MSG){
                HOTSTUFFDecideMsg *pmsg = (HOTSTUFFDecideMsg*)(entry->msg);
                msg_view = pmsg->view;
            }

            // if a msg arrives before the decide msg of the last round, it should go back to the end of the queue
            if(get_current_view(thd_id) < msg_view)
            {
                valid = false;
                while(!work_queue[0]->push(entry) && !simulation->is_done()) {}
            }
        }
#endif
    }

    UInt32 tcount = g_thread_cnt - g_execute_thd - g_checkpointing_thd;

    if (thd_id >= tcount && thd_id < (tcount + g_execute_thd))
    {
        // Thread for handling execute messages.
        uint64_t bid = ((get_expectedExecuteCount() + 2) - get_batch_size()) / get_batch_size();
        uint64_t qid = bid % indexSize;
        valid = work_queue[qid + 1]->pop(entry);
        if(valid){
            // cout << "DE" << entry->msg->txn_id << endl;
            fflush(stdout);
        }
    }
    else if (thd_id >= tcount + g_execute_thd)
    {
        // Thread for handling checkpoint messages.
        valid = work_queue[indexSize + 1]->pop(entry);
    }
#if RING_BFT
    else if (thd_id == tcount - 1)
    {
        // Thread for handling ring_commit messages.
        valid = work_queue[indexSize + 2]->pop(entry);
    }
#endif
    if (!valid)
    {
        // Allowing new transactions to be accessed by batching threads.
        if (thd_id > 0 && thd_id <= tcount - 1)
        {
            if(simulation->is_done()){
                return msg;
            }
            #if CONSENSUS == HOTSTUFF
            while(true){
                valid = new_txn_queue->pop(entry);
                if(valid){
                    if(!get_sent() && g_node_id == get_view_primary(get_current_view(thd_id))){
                        set_sent(true);
                        entry->msg->txn_id = get_current_view(thd_id);
                        break;
                    }
                    else{
                        valid = false;
                        while(!new_txn_queue->push(entry) && !simulation->is_done()) {}
                    }
                }
            }         
            #else
            valid = new_txn_queue->pop(entry);
            #endif //HOTSTUFF
        }
    }

    if (valid)
    {
        msg = entry->msg;
        assert(msg);
        uint64_t queue_time = get_sys_clock() - entry->starttime;
        INC_STATS(thd_id, work_queue_wait_time, queue_time);
        INC_STATS(thd_id, work_queue_cnt, 1);
        msg->wq_time = queue_time;
        DEBUG("Work Dequeue (%ld,%ld)\n", entry->txn_id, entry->batch_id);
        mem_allocator.free(entry, sizeof(work_queue_entry));
        INC_STATS(thd_id, work_queue_dequeue_time, get_sys_clock() - starttime);
    }
    return msg;
}


#if TEMP_QUEUE
 bool QWorkQueue::check_view(Message * msg){
    uint64_t msg_view = 0;
    if(msg->rtype == HOTSTUFF_PREP_MSG || msg->rtype == HOTSTUFF_GENERIC_MSG){
        HOTSTUFFPrepareMsg *pmsg = (HOTSTUFFPrepareMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_PREP_VOTE_MSG){
        HOTSTUFFPrepareVoteMsg *pmsg = (HOTSTUFFPrepareVoteMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_PRECOMMIT_VOTE_MSG){
        HOTSTUFFPreCommitVoteMsg *pmsg = (HOTSTUFFPreCommitVoteMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_COMMIT_VOTE_MSG){
        HOTSTUFFCommitVoteMsg *pmsg = (HOTSTUFFCommitVoteMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_NEW_VIEW_MSG){
        HOTSTUFFNewViewMsg *pmsg = (HOTSTUFFNewViewMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_PRECOMMIT_MSG){
        HOTSTUFFPreCommitMsg *pmsg = (HOTSTUFFPreCommitMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_COMMIT_MSG){
        HOTSTUFFCommitMsg *pmsg = (HOTSTUFFCommitMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_DECIDE_MSG){
        HOTSTUFFDecideMsg *pmsg = (HOTSTUFFDecideMsg*)(msg);
        msg_view = pmsg->view;
    }
    // if a msg arrives before the decide msg of the last round, it should go back to the end of the queue
    if(get_current_view(0) < msg_view)
    {
        work_queue_entry * entry = (work_queue_entry*)mem_allocator.align_alloc(sizeof(work_queue_entry));
        entry->msg = msg;
        entry->rtype = msg->rtype;
        entry->txn_id = msg->txn_id;
        entry->batch_id = msg->batch_id;
        entry->starttime = get_sys_clock();
        while(!temp_queue->push(entry) && !simulation->is_done()) {}
        return true;
    }
    return false;
}

void QWorkQueue::temp_store_newview(Message *msg){  
  work_queue_entry * entry = (work_queue_entry*)mem_allocator.align_alloc(sizeof(work_queue_entry));
  entry->msg = msg;
  entry->rtype = msg->rtype;
  entry->txn_id = msg->txn_id;
  entry->batch_id = msg->batch_id;
  entry->starttime = get_sys_clock();
  while(!new_view_queue->push(entry) && !simulation->is_done()) {}
}

void QWorkQueue::reenqueue(uint64_t instance_id, bool is_newview){
  bool valid = false;
  work_queue_entry * entry = NULL;
  uint64_t qid = 0;
  while(true){
      if(is_newview){
      valid = new_view_queue->pop(entry);
      }
      else{
        valid = temp_queue->pop(entry);
      }
    if(valid){
          while(!work_queue[qid]->push(entry) && !simulation->is_done()){}
          sem_post(&worker_queue_semaphore[qid]);
    }else{
      break;
    }
  }
}
#endif


#endif // ENABLE_PIPELINE == true

#endif // !MULTI_ON