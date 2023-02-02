#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include "global.h"
#include <vector>
#include <mutex>

#if TIMER_MANAGER

class PVPTimer{
public:
  bool waiting;
  uint64_t timer_length;
 	uint64_t start_time;
  uint64_t expiration_time;
 	uint64_t extra_length;
  std::mutex *timer_lock;
  uint64_t last_timeout_view;

 	PVPTimer(){}
  PVPTimer(uint64_t timer_length):waiting(false), timer_length(timer_length){
    last_timeout_view = 0;
 		timer_lock = new std::mutex;
 	}
   bool check_time_out(uint64_t current_view, uint64_t current_timer);
   void setTimer();
   void endTimer();
};

class TimerManager{
public:
  PVPTimer pvp_timer;
  std::mutex *tm_lock;
 	TimerManager(){
    pvp_timer = PVPTimer(INITIAL_TIMEOUT_LENGTH);
    tm_lock = new std::mutex;
  };
 	uint64_t check_timers(bool& timeout);
  void setTimer();
  void endTimer();
};

 /************************************/

extern TimerManager timer_manager;

#endif // TIMER_ON

#endif 