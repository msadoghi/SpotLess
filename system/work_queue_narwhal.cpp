/* Copyright (C) Exploratory Systems Laboratory - All Rights Reserved

Unauthorized copying, distribute, display, remix, derivative or deletion of any files or directories in this repository, via any medium is strictly prohibited. Proprietary and confidential
Written by Dakai Kang, October 2021.
*/

#include "work_queue.h"
#include "mem_alloc.h"
#include "query.h"
#include "message.h"
#include "client_query.h"
#include <boost/lockfree/queue.hpp>

#if NARWHAL
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

void QWorkQueue::init() {

  // Queue for worker thread 0.
  uint64_t effective_queue_cnt = 1;

  #if EXECUTION_THREAD 
    effective_queue_cnt += indexSize;
  #endif

  // A queue for checkpoint messages.  
  effective_queue_cnt++;

  // Additional instance queues
  effective_queue_cnt += get_multi_threads();

  cout << "Total queues: " << effective_queue_cnt << "\n";
  fflush(stdout);

  work_queue = new boost::lockfree::queue<work_queue_entry* > * [effective_queue_cnt];
  for(uint64_t i = 0; i < effective_queue_cnt; i++) {
    work_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }

  uint64_t num_multi_threads = get_multi_threads();

  new_txn_queue = new boost::lockfree::queue<work_queue_entry* > (0);

#if TEMP_QUEUE
  temp_queue = new boost::lockfree::queue<work_queue_entry* > * [num_multi_threads + 1];
  new_view_queue = new boost::lockfree::queue<work_queue_entry* > (0);
  for(uint64_t i = 0; i < num_multi_threads + 1; i++) {
    temp_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }
#endif

}


void QWorkQueue::enqueue(uint64_t thd_id, Message * msg,bool busy) {
  uint64_t starttime = get_sys_clock();
  assert(msg);
  DEBUG_M("QWorkQueue::enqueue work_queue_entry alloc\n");
  work_queue_entry * entry = (work_queue_entry*)mem_allocator.align_alloc(sizeof(work_queue_entry));
  entry->msg = msg;
  entry->rtype = msg->rtype;
  entry->txn_id = msg->txn_id;
  entry->batch_id = msg->batch_id;
  entry->starttime = get_sys_clock();
  
  assert(ISSERVER || ISREPLICA);
  DEBUG("Work Enqueue (%ld,%ld) %d\n",entry->txn_id,entry->batch_id,entry->rtype);
  uint64_t num_multi_threads = get_multi_threads();

  if(msg->rtype == CL_BATCH) {
    while(!new_txn_queue->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[g_thread_cnt - 3]);
    #endif
  }
  else if(msg->rtype == EXECUTE_MSG) { 
    uint64_t bid = ((msg->txn_id+2) - get_batch_size()) / get_batch_size();
    uint64_t qid = (bid % indexSize) + num_multi_threads + 1;
    while(!work_queue[qid]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[g_thread_cnt - 2]);
    execute_msg_heap_push(msg->txn_id);
    //if the next msg to execute is enqueued
    if(msg->txn_id == get_expectedExecuteCount()){  
      sem_post(&execute_semaphore);
    }
    #endif
    
  } 
  else if(msg->rtype == PBFT_CHKPT_MSG) {
    while(!work_queue[num_multi_threads + 1 + indexSize]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[g_thread_cnt - 1]);
    #endif
  } 
  else {
    uint64_t qid;
    if(msg->rtype == HOTSTUFF_PREP_MSG || msg->rtype == HOTSTUFF_GENERIC_MSG 
      || msg->rtype == HOTSTUFF_NEW_VIEW_MSG) {
      qid = num_multi_threads;
    } 
    else {
      qid = msg->return_node_id % num_multi_threads; 
    }
    while(!work_queue[qid]->push(entry) && !simulation->is_done()){}

    // cout << "ENQUEUED " << msg->txn_id << " " << qid << " " << msg->rtype << endl;
    fflush(stdout);
    
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[qid]);
    #endif
  } 

  INC_STATS(thd_id,work_queue_enqueue_time,get_sys_clock() - starttime);
  INC_STATS(thd_id,work_queue_enq_cnt,1);
}

Message * QWorkQueue::dequeue(uint64_t thd_id) {
  // cout << "DQ" << endl;
  uint64_t starttime = get_sys_clock();
  assert(ISSERVER || ISREPLICA);
  Message * msg = NULL;
  work_queue_entry * entry = NULL;

  bool valid = false;

  uint64_t num_multi_threads = get_multi_threads();

  if(thd_id <= num_multi_threads) {
    valid = work_queue[thd_id]->pop(entry);
  }

  UInt32 tcount = g_thread_cnt - g_execute_thd - g_checkpointing_thd; // 19 - 1 - 1 = 17

  if(thd_id >= tcount && thd_id < (tcount + g_execute_thd)) {
      // Thread for handling execute messages.
      uint64_t bid = ((get_expectedExecuteCount()+2) - get_batch_size()) /get_batch_size();
      uint64_t qid = (bid % indexSize) + num_multi_threads + 1;
      valid = work_queue[qid]->pop(entry);
  } 
  else if(thd_id >= tcount + g_execute_thd) {
    // Thread for handling checkpoint messages.
    valid = work_queue[indexSize + num_multi_threads + 1]->pop(entry);
  }

  if(!valid) {
    // Allowing new transactions to be accessed by batching threads.
    if(thd_id > num_multi_threads && thd_id < tcount) {
      if(simulation->is_done()){
        return msg;
      }
           while(true){
              valid = new_txn_queue->pop(entry);
              if(valid){
                  if(!get_sent() && g_node_id == get_current_view(thd_id) % g_node_cnt){
                      set_sent(true);
                      entry->msg->txn_id = get_next_idx_hotstuff();
                      break;
                  }
                  else{
                      valid = false;
                      while(!new_txn_queue->push(entry) && !simulation->is_done()) {}
                  }
              }
            }  
    } 
  }
  
  if(valid) 
  {
    msg = entry->msg;
    assert(msg);
    uint64_t queue_time = get_sys_clock() - entry->starttime;
    INC_STATS(thd_id,work_queue_wait_time,queue_time);
    INC_STATS(thd_id,work_queue_cnt,1);
    msg->wq_time = queue_time;
    DEBUG("Work Dequeue (%ld,%ld)\n",entry->txn_id,entry->batch_id);
    mem_allocator.free(entry,sizeof(work_queue_entry));
    INC_STATS(thd_id, work_queue_dequeue_time, get_sys_clock() - starttime);
  }

  return msg;
}

#if TEMP_QUEUE
 bool QWorkQueue::check_view(Message * msg){
    uint64_t msg_view = 0;
    if(msg->rtype == HOTSTUFF_PREP_MSG || msg->rtype == HOTSTUFF_GENERIC_MSG || msg->rtype == NARWHAL_PAYLOAD_MSG){
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
        printf("FUCK %lu %lu\n", msg->txn_id, msg_view);
        work_queue_entry * entry = (work_queue_entry*)mem_allocator.align_alloc(sizeof(work_queue_entry));
        entry->msg = msg;
        entry->rtype = msg->rtype;
        entry->txn_id = msg->txn_id;
        entry->batch_id = msg->batch_id;
        entry->starttime = get_sys_clock();
        if(msg->rtype != NARWHAL_PAYLOAD_MSG){
          while(!temp_queue[get_multi_threads()]->push(entry) && !simulation->is_done()) {}
        }else{
          while(!temp_queue[msg->return_node_id % get_multi_threads()]->push(entry) && !simulation->is_done()) {}
        }
        return true;
    }
    return false;
}

void QWorkQueue::reenqueue(uint64_t qid, bool is_newview){
  bool valid = false;
  work_queue_entry * entry = NULL;
  while(true){
    valid = temp_queue[qid]->pop(entry);
    if(valid){
      while(!work_queue[qid]->push(entry) && !simulation->is_done()){}
      sem_post(&worker_queue_semaphore[qid]);
    }else{
      break;
    }
  }
}
#endif

#endif // MUL
