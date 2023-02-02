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

#if PVP
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
        for(uint64_t i = 0; i < get_totInstances(); i++) {
            delete new_txn_queue[i];
            new_txn_queue[i] = nullptr;
        }
        delete []new_txn_queue;
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
  effective_queue_cnt += get_multi_threads()-1;

  cout << "Total queues: " << effective_queue_cnt << "\n";
  fflush(stdout);

  work_queue = new boost::lockfree::queue<work_queue_entry* > * [effective_queue_cnt];
  for(uint64_t i = 0; i < effective_queue_cnt; i++) {
    work_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }

  uint64_t num_instances = get_totInstances();
  new_txn_queue = new boost::lockfree::queue<work_queue_entry* > * [num_instances];

  for(uint64_t i = 0; i < num_instances; i++) {
    new_txn_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }

#if TEMP_QUEUE
  temp_queue = new boost::lockfree::queue<work_queue_entry* > * [num_instances];
  for(uint64_t i = 0; i < num_instances; i++) {
    temp_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
  }
#endif

  prior_queue = new boost::lockfree::queue<work_queue_entry* > * [MULTI_THREADS];
  for(uint64_t i = 0; i < MULTI_THREADS; i++) {
    prior_queue[i] = new boost::lockfree::queue<work_queue_entry* > (0);
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
  DEBUG("Work Enqueue (%ld,%ld) %d\n",entry->txn_id,entry->batch_id,entry->rtype);
  uint64_t num_multi_threads = get_multi_threads();
  uint64_t num_instances = get_totInstances();

  if(msg->rtype == CL_BATCH) {
  #if !EXCLUSIVE_BATCH
    uint64_t qid = msg->return_node_id % g_client_node_cnt % num_instances;
    while(!new_txn_queue[qid]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[num_multi_threads + 1]);
    #if SEPARATE
      sem_post(&worker_queue_semaphore[num_multi_threads]);
    #endif
    #endif
  #else
    uint64_t start_idx = idx;
    uint64_t min_cnt = 0;
    uint64_t _min_cnt = 0xFFFFF;
    while(true){
      clb_lock[idx].lock();
      if(clb_cnt[idx] <= min_cnt){
        while(!new_txn_queue[idx]->push(entry) && !simulation->is_done()) {}
        sem_post(&worker_queue_semaphore[num_multi_threads + 1]);
        sem_post(&worker_queue_semaphore[num_multi_threads]);
        clb_cnt[idx]++;
        // printf("EQ%lu\n", idx);
        // printf("CLB[%lu] = %lu\n", idx, clb_cnt[idx]);
        clb_lock[idx].unlock();
        idx = (idx+1) % num_instances;
        break;
      }
      else if(clb_cnt[idx] < _min_cnt){
        _min_cnt = clb_cnt[idx];
      }
      clb_lock[idx].unlock();
      idx = (idx+1) % num_instances;
      if(idx == start_idx){
        min_cnt = _min_cnt;
      }
    }
  #endif
  }
  else if(msg->rtype == EXECUTE_MSG) { 
    // printf("EE%lu\n", msg->txn_id);
    // fflush(stdout);
    uint64_t bid = ((msg->txn_id+2) - get_batch_size()) / get_batch_size();
    uint64_t qid = (bid % indexSize) + num_multi_threads;
    while(!work_queue[qid]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[num_multi_threads + 2]);
    execute_msg_heap_push(msg->txn_id);
    // if the next msg to execute is enqueued
    if(msg->txn_id == get_expectedExecuteCount()){  
      sem_post(&execute_semaphore);
    }
    #endif
    
  } 
  else if(msg->rtype == PBFT_CHKPT_MSG) {
    while(!work_queue[num_multi_threads + indexSize]->push(entry) && !simulation->is_done()) {}
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[num_multi_threads + 3]);
    #endif
  } 
  else if(msg->rtype == HOTSTUFF_GENERIC_MSG || msg->rtype == HOTSTUFF_PROPOSAL_MSG){
    uint64_t instance_id = msg->instance_id;
    uint64_t qid = instance_id % num_multi_threads;
    // printf("[A1]%lu$%lu$%lu\n", msg->txn_id, instance_id, qid);
    while(!prior_queue[qid]->push(entry) && !simulation->is_done()){}
    // printf("[A2]%lu$%lu$%lu\n", instance_id, msg->txn_id, qid);
    tb_lock[qid].lock();
    prior_cnt[qid]++;
    tb_lock[qid].unlock();
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[qid]);
    #endif
  }
  else{
    uint64_t instance_id = msg->instance_id;
    uint64_t qid = instance_id % num_multi_threads;
    // printf("[A3]%lu$%lu$%lu\n", msg->txn_id, instance_id, qid);
    // fflush(stdout);
    // assert(msg->instance_id == msg->txn_id / get_batch_size() % num_instances);
    while(!work_queue[qid]->push(entry) && !simulation->is_done()){}
    // printf("[A4]%lu$%lu$%lu\n", instance_id, msg->txn_id, qid);
    #if SEMA_TEST
    sem_post(&worker_queue_semaphore[qid]);
    #endif
  } 

  fflush(stdout);

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
  uint64_t num_instances = get_totInstances();

  if(thd_id < num_multi_threads) {
    tb_lock[thd_id].lock();
    if(prior_cnt[thd_id]){
      valid = prior_queue[thd_id]->pop(entry);
      if(valid){
        prior_cnt[thd_id]--;
      }
    }
    tb_lock[thd_id].unlock();
    if(!valid){
      valid = work_queue[thd_id]->pop(entry);
    }
    // else{
    //   printf("[B2]%lu$%lu\n", thd_id, entry->msg->txn_id);
    // }
    // if(valid){
    //   printf("[B4]%lu$%lu\n", thd_id, entry->msg->txn_id);
    // }
  }

  UInt32 tcount = g_thread_cnt - g_execute_thd - g_checkpointing_thd; // 19 - 1 - 1 = 17

  if(thd_id >= tcount && thd_id < (tcount + g_execute_thd)) {
      // Thread for handling execute messages.
      uint64_t bid = ((get_expectedExecuteCount()+2) - get_batch_size()) /get_batch_size();
      uint64_t qid = (bid % indexSize) + num_multi_threads;
      // cout << "[Z]";
      // fflush(stdout);
      valid = work_queue[qid]->pop(entry);
      // if(valid){
      //   printf("DE%lu\n", entry->msg->txn_id);
      //   fflush(stdout);
      // }else{
      //   cout << "Here" << endl;
      // }
  } 
  else if(thd_id >= tcount + g_execute_thd) {
    // Thread for handling checkpoint messages.
    valid = work_queue[indexSize + num_multi_threads]->pop(entry);
    if(valid){
        g_checkpointing_lock.lock();
        txn_chkpt_holding[thd_id % 2] = entry->msg->txn_id;
        is_chkpt_holding[thd_id % 2] = true;
        g_checkpointing_lock.unlock();
    }
  }
  
#if SEPARATE
#if PROPOSAL_THREAD
   if(!valid) {
    if(thd_id > num_multi_threads && thd_id < tcount) {
      // Generic
      while(true){
          if(simulation->is_done()){
            return msg;
          }
          if(get_incomplete_proposal_cnt(expectedInstance) != 0 && g_node_id == get_view_primary(get_current_view(expectedInstance), expectedInstance)){
            valid = true;
            uint64_t txn_id = (get_last_sent_view(expectedInstance) * num_instances + expectedInstance) * get_batch_size() + get_batch_size() - 1;
            entry = new work_queue_entry;
            entry->msg = Message::create_message(HOTSTUFF_GENERIC_MSG);
            entry->msg->rtype = HOTSTUFF_GENERIC_MSG_P;
            entry->msg->txn_id = txn_id;
            dec_incomplete_proposal_cnt(expectedInstance);
            expectedInstance = (expectedInstance + num_instances - 1) % num_instances;
            break;
          }
          expectedInstance = (expectedInstance + num_instances - 1) % num_instances;
      } 
    }
    else if(thd_id == num_multi_threads) {
      // Generic
#if !EXCLUSIVE_BATCH
      while(true){
          if(simulation->is_done()){
            return msg;
          }
          uint64_t view = get_next_send_view(proposalInstance);
          if(view <= get_current_view(proposalInstance) + ROUNDS_IN_ADVANCE){
            valid = new_txn_queue[proposalInstance % g_client_node_cnt]->pop(entry);
            if(valid){
              entry->msg->txn_id = view * num_instances + proposalInstance;
              // printf("DB%lu,%lu\n", proposalInstance, entry->msg->txn_id);
              set_last_sent_view(proposalInstance, view);
              inc_next_send_view(proposalInstance);
              // inc_incomplete_proposal_cnt(expectedInstance);
              proposalInstance = (proposalInstance + num_instances - 1) % num_instances;
              break;
            }
          }
          proposalInstance = (proposalInstance + num_instances - 1) % num_instances;
      }
#else
      while(true){
          if(simulation->is_done()){
            return msg;
          }
          uint64_t view = get_next_send_view(proposalInstance);
          if(view <= get_current_view(proposalInstance) + ROUNDS_IN_ADVANCE){
            // for(uint64_t j = 0; j< num_instances; j++){
            //   printf("KK[%lu] = %lu %lu\n", j, get_current_view(j), get_next_send_view(j));
            // }
            // printf("AA%lu\n", proposalInstance);
            uint64_t i = proposalInstance;
            while(true){
              clb_lock[i].lock();
              // if((clb_cnt[i] > 0 && i == proposalInstance) || clb_cnt[i] > 1 ){
              if(clb_cnt[i] > 0 && (i == proposalInstance || get_current_view(i) >= get_current_view(proposalInstance))){
                valid = new_txn_queue[i]->pop(entry);
                if(valid){
                  clb_cnt[i]--;
                  // printf("CLB[%lu] = %lu\n", idx, clb_cnt[idx]);
                  clb_lock[i].unlock();
                  // printf("DQ%lu %lu\n", i, proposalInstance);
                  break;
                }
              }
              clb_lock[i].unlock();
              i = (i+1) % num_instances;
              if(i == proposalInstance){
                break;
              }
            }
            if(valid){
              entry->msg->txn_id = view * num_instances + proposalInstance;
              // printf("DB%lu,%lu\n", proposalInstance, entry->msg->txn_id);
              set_last_sent_view(proposalInstance, view);
              inc_next_send_view(proposalInstance);
              // inc_incomplete_proposal_cnt(expectedInstance);
              proposalInstance = (proposalInstance + num_instances - 1) % num_instances;
              break;
            }
          }
          proposalInstance = (proposalInstance + num_instances - 1) % num_instances;
      }
#endif
    }
  }
#else
  if(!valid) {
    // Allowing new transactions to be accessed by batching threads.
    if(thd_id >= num_multi_threads && thd_id < tcount) {
      if(simulation->is_done()){
        return msg;
      }
      while(true){
        uint k = 0;
        while(k<num_instances){
          k++;
          // Generic
          if(get_incomplete_proposal_cnt(expectedInstance) != 0 && g_node_id == get_view_primary(get_current_view(expectedInstance), expectedInstance)){
            valid = true;
            uint64_t txn_id = (get_last_sent_view(expectedInstance) * num_instances + expectedInstance) * get_batch_size() + get_batch_size() - 1;
            entry = new work_queue_entry;
            entry->msg = Message::create_message(HOTSTUFF_GENERIC_MSG);
            entry->msg->rtype = HOTSTUFF_GENERIC_MSG_P;
            entry->msg->txn_id = txn_id;
            dec_incomplete_proposal_cnt(expectedInstance);
            expectedInstance = (expectedInstance + num_instances - 1) % num_instances;
            break;
          }
          expectedInstance = (expectedInstance + num_instances - 1) % num_instances;
        }
        if(valid)
          break;
        // Proposal
        k = 0;
        while(k<num_instances){
          k++;
          uint64_t view = get_next_send_view(expectedInstance);
          if(view <= get_current_view(expectedInstance) + ROUNDS_IN_ADVANCE){
            valid = new_txn_queue[expectedInstance % g_client_node_cnt]->pop(entry);
            if(valid){
              entry->msg->txn_id = view * num_instances + expectedInstance;
              set_last_sent_view(expectedInstance, view);
              inc_next_send_view(expectedInstance);
              inc_incomplete_proposal_cnt(expectedInstance);
              expectedInstance = (expectedInstance + num_instances - 1) % num_instances;
              break;
            }
          }
          expectedInstance = (expectedInstance + num_instances - 1) % num_instances;
        }
        if(valid)
          break;
      } 
    }
  }
#endif
#endif

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
    INC_STATS(thd_id,work_queue_dequeue_time,get_sys_clock() - starttime);
  }

  return msg;
}

#if TEMP_QUEUE
bool QWorkQueue::check_view(Message * msg){
    uint64_t msg_view = 0;
    uint64_t instance_id = msg->instance_id;
    if(msg->rtype == HOTSTUFF_NEW_VIEW_MSG || msg->rtype == HOTSTUFF_PROPOSAL_MSG){
      return false;
    }
    if(msg->rtype == HOTSTUFF_PREP_MSG){
        HOTSTUFFPrepareMsg *pmsg = (HOTSTUFFPrepareMsg*)(msg);
        msg_view = pmsg->view;
    }else if(msg->rtype == HOTSTUFF_GENERIC_MSG){
        HOTSTUFFGenericMsg *pmsg = (HOTSTUFFGenericMsg*)(msg);
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
    }
    // else if(msg->rtype == HOTSTUFF_NEW_VIEW_MSG){
    //     HOTSTUFFNewViewMsg *pmsg = (HOTSTUFFNewViewMsg*)(msg);
    //     msg_view = pmsg->view;
    // }
    else if(msg->rtype == HOTSTUFF_PRECOMMIT_MSG){
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
    if(get_current_view(instance_id) < msg_view)
    {
        work_queue_entry * entry = (work_queue_entry*)mem_allocator.align_alloc(sizeof(work_queue_entry));
        entry->msg = msg;
        entry->rtype = msg->rtype;
        entry->txn_id = msg->txn_id;
        entry->batch_id = msg->batch_id;
        entry->starttime = get_sys_clock();
        while(!temp_queue[instance_id]->push(entry) && !simulation->is_done()) {}
        return true;
    }
    return false;
}

void QWorkQueue::reenqueue(uint64_t instance_id){
  	bool valid = false;
	  work_queue_entry * entry = NULL;
    uint64_t qid = instance_id % get_multi_threads();
	  while(true){
      valid = temp_queue[instance_id]->pop(entry);
		  if(valid){
          if(entry->msg->rtype == HOTSTUFF_GENERIC_MSG || entry->msg->rtype == HOTSTUFF_PROPOSAL_MSG){
            while(!prior_queue[qid]->push(entry) && !simulation->is_done()){}
            tb_lock[qid].lock();
            prior_cnt[qid]++;
            tb_lock[qid].unlock();
          }else{
            while(!work_queue[qid]->push(entry) && !simulation->is_done()){}
          }
          sem_post(&worker_queue_semaphore[qid]);
		  }else{
			  break;
		  }
	  }
}
#endif

#endif // PVP