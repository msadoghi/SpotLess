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
//[Dakai] A new QWorkQueue class, which is different from the one existing in resdb.
void QWorkQueue::init() {

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
  uint64_t num_instances = get_multi_threads();

  if(msg->rtype == CL_BATCH) {
    if(isPrimary(g_node_id)) {
      // cout << "Placing \n";
      while(!new_txn_queue->push(entry) && !simulation->is_done()) {}
      // cout << "Placed \n";
    } else {
      ClientQueryBatch *cl = (ClientQueryBatch*)msg;
      uint64_t qid = cl->return_node % num_instances;
      while(!work_queue[qid]->push(entry) && !simulation->is_done()) {}
    }
  }
  else if(msg->rtype == EXECUTE_MSG) { 
    uint64_t bid = ((msg->txn_id+2) - get_batch_size()) / get_batch_size();
    uint64_t qid = (bid % indexSize) + num_instances;
    while(!work_queue[qid]->push(entry) && !simulation->is_done()) {}
  } 
  else if(msg->rtype == PBFT_CHKPT_MSG) {
    while(!work_queue[num_instances + indexSize]->push(entry) && !simulation->is_done()) {}
  } 
  else {
    //cout << "Rtype: " << msg->rtype << " :: Txn: " << msg->txn_id << "\n";
    //fflush(stdout);
    uint64_t qid;
    if(msg->rtype == BATCH_REQ) {
      qid = (((msg->txn_id+3) - get_batch_size()) / get_batch_size()) % num_instances;
    } 
   #if CONSENSUS == PBFT
    else if(msg->rtype == PBFT_PREP_MSG || msg->rtype == PBFT_COMMIT_MSG) {
      qid = (((msg->txn_id+1) - get_batch_size()) / get_batch_size()) % num_instances; 
    } else {
      qid = 0;
    }
   #else
    else {
      qid = (((msg->txn_id+1) - get_batch_size()) / get_batch_size()) % num_instances; 
      //cout << "Tid: " << msg->txn_id << " :: Qid: " << qid << "\n";
      //fflush(stdout); 
    }
   #endif 
    
    while(!work_queue[qid]->push(entry) && !simulation->is_done()){}
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

  if(thd_id < get_multi_threads()) {
      valid = work_queue[thd_id]->pop(entry);
  }

  uint64_t num_instances = get_multi_threads();
  
  //[Dakai] Change g_btorder_thd to g_checkpointing_thd
  //UInt32 tcount = g_thread_cnt - g_execute_thd - g_btorder_thd;
  UInt32 tcount = g_thread_cnt - g_execute_thd - g_checkpointing_thd; //19 = 21 - 1 - 1

  if(thd_id >= tcount && thd_id < (tcount + g_execute_thd)) {
      // Thread for handling execute messages.
      uint64_t bid = ((get_expectedExecuteCount()+2) - get_batch_size()) /get_batch_size();
      uint64_t qid = (bid % indexSize) + num_instances;
      valid = work_queue[qid]->pop(entry);
  } 
  else if(thd_id >= tcount + g_execute_thd) {
    // Thread for handling checkpoint messages.
    valid = work_queue[indexSize + num_instances]->pop(entry);
  }

  if(!valid) {
    // Allowing new transactions to be accessed by batching threads.
    if(thd_id >= get_multi_threads() && thd_id < tcount) {
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

    mem_allocator.free(entry,sizeof(work_queue_entry));
    INC_STATS(thd_id,work_queue_dequeue_time,get_sys_clock() - starttime);
  }

  return msg;
}

#endif // MULTI_ON
