#include "global.h"
#include "timer_manager.h"

#if TIMER_MANAGER

bool PVPTimer::check_time_out(uint64_t current_view, uint64_t current_time){
	if(current_view < CRASH_VIEW)
 		return false;
 	this->timer_lock->lock();
    if(waiting && current_time > expiration_time){
		cout << current_time << "|" << expiration_time << "|" << start_time << endl;
		this->waiting = false;
		if(last_timeout_view < current_view - 1){
			this->extra_length = this->timer_length;
		}
		this->timer_length += this->extra_length;
		if(timer_length > MAX_TIMER_LEN){
			timer_length = MAX_TIMER_LEN;
		}
		this->last_timeout_view = current_view;
 		cout << "TL " << this->timer_length << endl;
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
		if((current_time - this->start_time) < (this->timer_length>>2)){
 			this->timer_length = this->timer_length >> 1;
 			cout << "LT " << timer_length << endl;
		}
	}
	this->waiting = false;
	this->timer_lock->unlock();
}

uint64_t TimerManager::check_timers(bool& timeout){
	this->tm_lock->lock();
	uint64_t current_time = get_sys_clock();
#if NEW_DIVIDER && CRASH_DIVIDER == 3
	uint64_t gid = get_view_primary(get_current_view(0));
#endif	
 	if(!simulation->is_warmup_done()){
		timeout = false;
	}
#if NEW_DIVIDER && CRASH_DIVIDER == 3
	else if(!(gid % DIV1 < LIMIT1 && gid % DIV2 != LIMIT2 && gid<124)){
#else
	else if(get_view_primary(get_current_view(0)) % CRASH_DIVIDER != CRASH_ID){
#endif
		timeout = false;
	}else if(pvp_timer.check_time_out(get_current_view(0), current_time)){
		timeout = true;
	}
	this->tm_lock->unlock();
	return 0;
}

void TimerManager::setTimer(){
	if(!simulation->is_warmup_done()){
 		return;
 	}
 	this->tm_lock->lock();
 	pvp_timer.setTimer();
 	this->tm_lock->unlock();
 	return;
}

void TimerManager::endTimer(){
	this->tm_lock->lock();
 	pvp_timer.endTimer();
 	this->tm_lock->unlock();
}

TimerManager timer_manager;

#endif // TIMER_ON