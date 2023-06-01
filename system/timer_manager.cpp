#include "global.h"
#include "timer_manager.h"

#if TIMER_MANAGER

bool PVPTimer::check_time_out(uint64_t current_view, uint64_t current_time){
	if(current_view <= CRASH_VIEW)
		return false;
	this->timer_lock->lock();
    if(waiting && current_time > expiration_time){
		// cout << current_time << "|" << expiration_time << "|" << start_time << endl;
		this->waiting = false;
		if(last_timeout_view < current_view - 1){
			this->extra_length = this->timer_length;
		}
		this->timer_length += this->extra_length;
		if(this->timer_length > MAX_TIMER_LEN){
			this->timer_length = MAX_TIMER_LEN;
		}
		this->last_timeout_view = current_view;
		// cout << "TL " << this->timer_length << endl;
		this->timer_lock->unlock();
        return true;
    }
	this->timer_lock->unlock();
    return false;
}

void PVPTimer::setTimer(){
	if(!simulation->is_warmup_done()){
		return;
	}
	this->timer_lock->lock();
	uint64_t current_time = get_sys_clock();
    this->waiting = true;
	this->start_time = current_time;
    this->expiration_time = this->start_time + this->timer_length;
	this->timer_lock->unlock();
}

void PVPTimer::endTimer(){
	this->timer_lock->lock();
	uint64_t current_time = get_sys_clock();
	if(this->waiting){
		if((current_time - this->start_time) < (this->timer_length>>3)){
			this->timer_length = this->timer_length >> 1;
			// printf("[X]%lu %lu\n", this->timer_length, (current_time - this->start_time));
		}
	}
    this->waiting = false;
	this->timer_lock->unlock();
}

TimerManager::TimerManager(uint64_t thd_id){
	for(uint64_t instance_id = thd_id; instance_id < get_totInstances(); instance_id+=get_multi_threads()){
		pvp_timers[instance_id] = PVPTimer(INITIAL_TIMEOUT_LENGTH);
	}
	tm_lock = new std::mutex;
}

uint64_t TimerManager::check_timers(bool& timeout){
	this->tm_lock->lock();
	timeout = false;
	uint64_t current_time = get_sys_clock();
	if(!simulation->is_warmup_done() || current_time < min_exp_time){
		this->tm_lock->unlock();
		return 0;
	}
	for(auto it = pvp_timers.begin(); it != pvp_timers.end(); it++){
		uint64_t value = get_view_primary(get_current_view(it->first), it->first);
		#if NEW_DIVIDER && FAIL_DIVIDER == 3
		if(!(value % DIV1 < LIMIT1 && value % DIV2 != LIMIT2))
		#else
		if(value % FAIL_DIVIDER != FAIL_ID )
		#endif
			continue;
		if(it->second.check_time_out(get_current_view(it->first), current_time)){
			timeout = true;
			this->tm_lock->unlock();
			return it->first;
		}
	}
	this->tm_lock->unlock();
	return 0;
}

void TimerManager::setTimer(uint64_t instance_id){
	if(!simulation->is_warmup_done()){
		return;
	}
	this->tm_lock->lock();
	bool reorder = false;
	if(instance_id == min_id){
		reorder = true;
	}
	pvp_timers[instance_id].setTimer();
	if(reorder || pvp_timers[instance_id].expiration_time < min_exp_time){
		min_id = instance_id;
		min_exp_time = pvp_timers[instance_id].expiration_time;
		for(auto it = pvp_timers.begin(); it != pvp_timers.end(); it++){
			if(it->second.waiting && it->second.expiration_time < min_exp_time){
				min_id = it->first;
				min_exp_time = it->second.expiration_time;
			}
		}
	}
	this->tm_lock->unlock();
	return;
}

void TimerManager::endTimer(uint64_t instance_id){
	if(!simulation->is_warmup_done()){
		return;
	}
	this->tm_lock->lock();
	bool reorder = false;
	if(instance_id == min_id){
		reorder = true;
	}
	pvp_timers[instance_id].endTimer();
	if(reorder){
		min_id = instance_id;
		min_exp_time = 0xFFFFFFF;
		for(auto it = pvp_timers.begin(); it != pvp_timers.end(); it++){
			if(it->second.waiting && it->second.expiration_time < min_exp_time){
				min_id = it->first;
				min_exp_time = it->second.expiration_time;
			}
		}
	}
	this->tm_lock->unlock();
	return;
}

TimerManager timer_manager[MULTI_INSTANCES];

#endif
