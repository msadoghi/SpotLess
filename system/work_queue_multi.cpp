/* Copyright (C) Exploratory Systems Laboratory - All Rights Reserved

Unauthorized copying, distribute, display, remix, derivative or deletion of any files or directories in this repository, via any medium is strictly prohibited. Proprietary and confidential
Written by Suyash Gupta, October 2018.
*/

#include "work_queue.h"
#include "mem_alloc.h"
#include "query.h"
#include "message.h"
#include "client_query.h"
#include <boost/lockfree/queue.hpp>


#if MULTI_ON
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
        for(uint64_t i=0; i<effective_queue_cnt; i++){
            if(work_queue[i]){
                delete work_queue[i];
                work_queue[i] = nullptr;
            }
        }
        delete work_queue;
        work_queue = nullptr;
    }
    if(new_txn_queue){
        delete new_txn_queue;
        new_txn_queue = nullptr;
    }
}

//[Dakai] A new QWorkQueue class, which is different from the one existing in resdb.
void QWorkQueue::init() {

  last_sched_dq = NULL;
  sched_ptr = 0;
  seq_queue = new boost::lockfree::queue<work_queue_entry* > (0);

  // Queue for worker thread 0.
  uint64_t effective_queue_cnt = 1;

  #if EXECUTION_THREAD 
    effective_queue_cnt += indexSize;
  #endif

  // A queue for checkpoint messages.  
  effective_queue_cnt++;

  // Additional instance queues
  effective_queue_cnt += get_multi_threads()-1;

  cout << "Total queues: " << effective_queue_cnt << "\n";
  fflush(stdout);

  work_queue = new boost::lockfree::queue<work_queue_entry* > * [effective_queue_cnt];
  for(uint64_t i = 0; i < effective_queue_cnt; i++) {
    work_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }

  new_txn_queue = new boost::lockfree::queue<work_queue_entry* >(0);
  sched_queue = new boost::lockfree::queue<work_queue_entry* > * [g_node_cnt];
  for ( uint64_t i = 0; i < g_node_cnt; i++) {
    sched_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }

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
  #if KDK_DEBUG2
  printf("Work Enqueue (%ld,%ld) %d\n",entry->txn_id,entry->batch_id,entry->rtype);
  #endif
  uint64_t num_multi_threads = get_multi_threads();

  if(msg->rtype == CL_BATCH) {
    if(isPrimary(g_node_id)) {
      while(!new_txn_queue->push(entry) && !simulation->is_done()) {}
      #if SEMA_TEST
        sem_post(&worker_queue_semaphore[num_multi_threads]);
      #endif
    }
  }
  else if(msg->rtype == EXECUTE_MSG) { 
    uint64_t bid = ((msg->txn_id+2) - get_batch_size()) / get_batch_size();
    uint64_t qid = (bid % indexSize) + num_multi_threads;
    while(!work_queue[qid]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
      sem_post(&worker_queue_semaphore[num_multi_threads + CL_THD_CNT]);
      execute_msg_heap_push(msg->txn_id);
      //if the next msg to execute is enqueued
      if(msg->txn_id == get_expectedExecuteCount()){
          sem_post(&execute_semaphore);
      }
    #endif
    // if(bid % 100 == 0){
    //   cout << "[A]" << bid*100 << endl;
    // }
  } 
  else if(msg->rtype == PBFT_CHKPT_MSG) {
    while(!work_queue[num_multi_threads + indexSize]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
      sem_post(&worker_queue_semaphore[num_multi_threads + 1 + CL_THD_CNT]);
    #endif
  } 
  else {
    //cout << "Rtype: " << msg->rtype << " :: Txn: " << msg->txn_id << "\n";
    //fflush(stdout);
    uint64_t qid;
    if(msg->rtype == BATCH_REQ) {
      qid = (((msg->txn_id+3) - get_batch_size()) / get_batch_size()) % num_multi_threads;
    } 
    else if(msg->rtype == PBFT_PREP_MSG || msg->rtype == PBFT_COMMIT_MSG) {
      qid = (((msg->txn_id+1) - get_batch_size()) / get_batch_size()) % num_multi_threads; 
    } else {
      qid = 0;
    }
    
    while(!work_queue[qid]->push(entry) && !simulation->is_done()){}
    #if SEMA_TEST
      sem_post(&worker_queue_semaphore[qid]);
    #endif
  } 

  INC_STATS(thd_id,work_queue_enqueue_time,get_sys_clock() - starttime);
  INC_STATS(thd_id,work_queue_enq_cnt,1);
}

Message * QWorkQueue::dequeue(uint64_t thd_id) {
  uint64_t starttime = get_sys_clock();
  assert(ISSERVER || ISREPLICA);
  Message * msg = NULL;
  work_queue_entry * entry = NULL;

  bool valid = false;

  uint64_t num_multi_threads = get_multi_threads();

  if(thd_id < num_multi_threads) {
      valid = work_queue[thd_id]->pop(entry);
  }

  if(thd_id == num_multi_threads + CL_THD_CNT) {
      // Thread for handling execute messages.
      uint64_t bid = ((get_expectedExecuteCount()+2) - get_batch_size()) /get_batch_size();
      uint64_t qid = (bid % indexSize) + num_multi_threads;
      valid = work_queue[qid]->pop(entry);
  } 
  else if(thd_id >= num_multi_threads + 1 + CL_THD_CNT) {
    // Thread for handling checkpoint messages.
    valid = work_queue[indexSize + num_multi_threads]->pop(entry);
  }

  if(!valid) {
    // Allowing new transactions to be accessed by batching threads.
    if(thd_id >= num_multi_threads && thd_id < num_multi_threads + CL_THD_CNT) {
      valid = new_txn_queue->pop(entry);
    } 
  }
  
  if(valid) {
    msg = entry->msg;
    assert(msg);
    uint64_t queue_time = get_sys_clock() - entry->starttime;
    INC_STATS(thd_id,work_queue_wait_time,queue_time);
    INC_STATS(thd_id,work_queue_cnt,1);
    
    msg->wq_time = queue_time;
    #if KDK_DEBUG2
    printf("Work Dequeue (%ld,%ld)\n",entry->txn_id,entry->batch_id);
    #endif
    mem_allocator.free(entry,sizeof(work_queue_entry));
    INC_STATS(thd_id,work_queue_dequeue_time,get_sys_clock() - starttime);
  }

  return msg;
}

#endif // MULTI_ON
