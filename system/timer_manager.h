#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include "global.h"
#include <vector>
#include <mutex>

#if TIMER_MANAGER

class SpotLessTimer{
public:
    bool waiting;
    uint64_t timer_length;
	uint64_t start_time;
    uint64_t expiration_time;
    uint64_t last_timeout_view;
	uint64_t extra_length;
	std::mutex *timer_lock;

	SpotLessTimer(){}
    SpotLessTimer(uint64_t timer_length):waiting(false), timer_length(timer_length){
		last_timeout_view = 0;
		timer_lock = new std::mutex;
	}
    bool check_time_out(uint64_t current_view, uint64_t current_timer);
    void setTimer();
	void endTimer();
};

class TimerManager{
public:
	std::map<uint64_t, SpotLessTimer> spotless_timers;
	std::mutex *tm_lock;
	uint64_t min_id = 0;
	uint64_t min_exp_time = 0x3FFFFFFF;
	TimerManager(){};
	TimerManager(uint64_t thd_id);
	uint64_t check_timers(bool& timeout);
	SpotLessTimer& operator[](uint64_t instance_id){
		return spotless_timers[instance_id];
	}
	void setTimer(uint64_t instance_id);
	void endTimer(uint64_t instance_id);
};

/************************************/

extern TimerManager timer_manager[MULTI_INSTANCES];

#endif 

#endif