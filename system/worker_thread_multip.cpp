/* Copyright (C) Exploratory Systems Laboratory - All Rights Reserved

Unauthorized copying, distribute, display, remix, derivative or deletion of any files or directories in this repository, via any medium is strictly prohibited. Proprietary and confidential
Written by Suyash Gupta, October 2018.
*/

#include "global.h"
#include "message.h"
#include "thread.h"
#include "worker_thread.h"
#include "txn.h"
#include "wl.h"
#include "query.h"
#include "ycsb_query.h"
#include "math.h"
#include "helper.h"
#include "msg_thread.h"
#include "msg_queue.h"
#include "work_queue.h"
#include "message.h"
//#include "abort_queue.h"
#include "timer.h"


#if MULTI_ON && CONSENSUS == PBFT

// #if MAL_PRIMARY

// /* To simulate the behavior of a malicious primary we are just 
// * waiting for a request after the first checkpoint, that is, with 
// * txn_id = 7 and then we perform the attack. 
// * As we want the mal primary to send different requests to f replicas, 
// * so we make mal primary to store the request at txn_id = 6 and then 
// * send this stored request to the f replicas. 
// */
// void WorkerThread::mal_check(ClientQueryBatch *clbtch, string hsh) {
// 	if(clbtch->txn_id == txn_per_chkpt()) {
// 	  char * buf = create_msg_buffer(clbtch);
// 	  Message * deepMsg = deep_copy_msg(buf, clbtch);
// 	  oldCQry = clbtch;
// 	  delete_msg_buffer(buf);

// 	  // Storing the hash.
// 	  oldCQryHash = hsh;
// 	}
// 	else if(clbtch->txn_id == txn_per_chkpt()+1) {
// 	// Store in BatchRequests message and send to only f.  
// 	}
// }

// #endif




#endif // MULTI_ON  && CONSENSUS == PBFT


#if MULTI_ON


// execute messages and sends client response message
RC WorkerThread::process_execute_msg(Message * msg)
{
	uint64_t starttime = get_sys_clock();

	ExecuteMessage *emsg = (ExecuteMessage*)msg;
	#if PROCESS_PRINT
	cout << "EXECUTE "<< msg->txn_id << " Thd: "<< get_thd_id() << endl;
	fflush(stdout);
	#endif
	vector<uint64_t> alltxn, allts;
	
	// Execute transactions in a shot.
	
	// This messages sends response to the client.
	Message *rsp = Message::create_message(CL_RSP);
	ClientResponseMessage *crsp = (ClientResponseMessage *)rsp;
	crsp->init();

	uint64_t i;
	for(i = emsg->index; i<emsg->end_index-4; i++) {
	  //cout << "i: " << i << " :: nxt: " << curr_next_index() << "\n";
	  //fflush(stdout);

	  TxnManager * tman = get_transaction_manager(i, 0);
	        
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
	for(; i<emsg->end_index; i++) {
          TxnManager * tman = get_transaction_manager(i,0);
          while(true) {
            bool ready = tman->unset_ready();	
            if(!ready) {
              continue;
            } else {
	      break;
	    } }

          // Execute the transaction		
          tman->run_txn();

          // Commit the results.
          tman->commit();

		INC_STATS(get_thd_id(), txn_cnt, 1);

	  crsp->copy_from_txn(tman);

	  // Making this txn man available.
	  bool ready = tman->set_ready();
	  assert(ready);
       	}

        // Last Transaction of the batch.
       	txn_man = get_transaction_manager(i, 0);
	while(true) {
	  bool ready = txn_man->unset_ready();	
	  if(!ready) {
	    continue;
	  } else {
	    break;
	  } }

	// Execute the transaction
	txn_man->run_txn();

	// Commit the results.
    txn_man->commit();
	INC_STATS(get_thd_id(), txn_cnt, 1);
	crsp->copy_from_txn(txn_man);

	// Increment.
	inc_next_index(get_batch_size());

	// Send the response.
	vector<string> emptyvec;
	vector<uint64_t> dest;
	dest.push_back(txn_man->client_id);
	msg_queue.enqueue(get_thd_id(), crsp, dest);
	dest.clear();

	INC_STATS(_thd_id,tput_msg,1);
	INC_STATS(_thd_id,msg_cl_out,1);
		
	// Send checkpoint messages.
	send_checkpoints(msg->txn_id+1);

	//// Release txn_man for last txn of the batch.
	//bool ready = txn_man->set_ready();
	//assert(ready);

	// Setting the next expected prepare message id.
	set_expectedExecuteCount(get_batch_size() + msg->txn_id);

	// End the execute counter.
	INC_STATS(get_thd_id(), time_execute, get_sys_clock() - starttime);

	//txn_man = get_transaction_manager(msg->txn_id, 0);
	//ready = txn_man->unset_ready();
	//assert(ready);

	// Updating the last txn.
	// last_txn_processed = msg->txn_id;

	return RCOK;		
}


#endif // MULTI_ON 

