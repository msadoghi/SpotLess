#include "message.h"
#include <mutex>

#if TIMER_ON

class Timer
{
	uint64_t timestamp;
	string hash;
	Message *msg;

public:
	uint64_t get_timestamp();
	string get_hash();
	Message *get_msg();
	void set_data(uint64_t tst, string hsh, Message *cqry);
};

// Timer for servers
class ServerTimer
{
	// Stores time of arrival for each transaction.
	std::vector<Timer *> txn_map;
	bool timer_state;
	std::mutex tlock;
	
public:

#if PVP_RECOVERY
	bool waiting_prepare;
	uint64_t last_new_view_time;
	bool timeout = false;
#endif

#if CONSENSUS == HOTSTUFF && PVP_RECOVERY
	bool checkTimer(Timer*& ptimer);
#endif

	void startTimer(string digest, Message *clqry);
	void endTimer(string digest);
	bool checkTimer();

	void pauseTimer();
	void resumeTimer();
	Timer *fetchPendingRequests(uint64_t idx);
	uint64_t timerSize();
	void removeAllTimers();
};

// Timer for clients.
class ClientTimer
{
	// Stores time of arrival for each transaction.
	std::vector<Timer *> txn_map;
	std::mutex tlock;
public:
	void startTimer(uint64_t timestp, ClientQueryBatch *cqry);
	void endTimer(uint64_t timestp);
	bool checkTimer(ClientQueryBatch *&cbatch);
	Timer *fetchPendingRequests();
	void removeAllTimers();
};

/************************************/

extern ClientTimer *client_timer;
#if !PVP
extern ServerTimer *server_timer;
#else
extern ServerTimer *server_timer[MULTI_INSTANCES];
#endif

#endif // TIMER_ON
