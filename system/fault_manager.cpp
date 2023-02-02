#include "global.h"
 #include "fault_manager.h"

 bool FaultVoters::add_voter(uint64_t view, uint64_t voter){
     flock->lock();
     if(voters[view].size() >= 2*g_min_invalid_nodes + 1){
         flock->unlock();
         return false;
     }
     voters[view].insert(voter);
     if(voters[view].size() >= 2*g_min_invalid_nodes + 1){
         flock->unlock();
         return true;
     }
     flock->unlock();
     return false;
 }

bool FaultManager::sufficient_voters(uint64_t view, uint64_t voter_id){
     return fault_voters.add_voter(view, voter_id);
} 