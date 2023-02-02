#ifndef FAULT_MANAGER_H
 #define FAULT_MANAGER_H
 #include "global.h"
 #include <map>
 #include <vector>
 #include <set>
 #include <mutex>

 class HOTSTUFFNewViewMsg;

 class FaultVoters{
 public:
     std::map<uint64_t, std::set<uint64_t>> voters;
     std::mutex *flock;
     FaultVoters(){
         flock = new std::mutex;
     }
     bool add_voter(uint64_t view, uint64_t voter);
 };

 class FaultManager{
 public:
    FaultVoters fault_voters;

    bool sufficient_voters(uint64_t view, uint64_t voter_id);
 };

 #endif 