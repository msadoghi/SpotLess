#ifndef _WORK_QUEUE_H_
#define _WORK_QUEUE_H_

#include "global.h"
#include <queue>
#include <boost/lockfree/queue.hpp>

class BaseQuery;
class Workload;
class Message;

struct work_queue_entry
{
    Message *msg;
    uint64_t batch_id;
    uint64_t txn_id;
    RemReqType rtype;
    uint64_t starttime;
};

struct CompareSchedEntry
{
    bool operator()(const work_queue_entry *lhs, const work_queue_entry *rhs)
    {
        if (lhs->batch_id == rhs->batch_id)
            return lhs->starttime > rhs->starttime;
        return lhs->batch_id < rhs->batch_id;
    }
};
struct CompareWQEntry
{
#if PRIORITY == PRIORITY_FCFS
    bool operator()(const work_queue_entry *lhs, const work_queue_entry *rhs)
    {
        return lhs->starttime < rhs->starttime;
    }
#elif PRIORITY == PRIORITY_ACTIVE
    bool operator()(const work_queue_entry *lhs, const work_queue_entry *rhs)
    {
        if (lhs->rtype == CL_QRY && rhs->rtype != CL_QRY)
            return true;
        if (rhs->rtype == CL_QRY && lhs->rtype != CL_QRY)
            return false;
        return lhs->starttime < rhs->starttime;
    }
#elif PRIORITY == PRIORITY_HOME
    bool operator()(const work_queue_entry *lhs, const work_queue_entry *rhs)
    {
        if (ISLOCAL(lhs->txn_id) && !ISLOCAL(rhs->txn_id))
            return true;
        if (ISLOCAL(rhs->txn_id) && !ISLOCAL(lhs->txn_id))
            return false;
        return lhs->starttime < rhs->starttime;
    }
#endif
};

class QWorkQueue
{
public:
    ~QWorkQueue();
    void init();
    void release();
    void enqueue(uint64_t thd_id, Message *msg, bool busy);
    Message *dequeue(uint64_t thd_id);
    void sched_enqueue(uint64_t thd_id, Message *msg);
    Message *sched_dequeue(uint64_t thd_id);
    void sequencer_enqueue(uint64_t thd_id, Message *msg);
    Message *sequencer_dequeue(uint64_t thd_id);
#if TEMP_QUEUE
    bool check_view(Message * msg);
    void reenqueue(uint64_t instance_id);
#endif
    uint64_t get_cnt() { return get_wq_cnt() + get_rem_wq_cnt() + get_new_wq_cnt(); }
    uint64_t get_wq_cnt() { return 0; }
    uint64_t get_sched_wq_cnt() { return 0; }
    uint64_t get_rem_wq_cnt() { return 0; }
    uint64_t get_new_wq_cnt() { return 0; }

private:
    boost::lockfree::queue<work_queue_entry *> **work_queue = nullptr;
    boost::lockfree::queue<work_queue_entry *> **new_txn_queue = nullptr;

#if TEMP_QUEUE
    boost::lockfree::queue<work_queue_entry *> **temp_queue = nullptr;
#endif

    boost::lockfree::queue<work_queue_entry *> **prior_queue = nullptr;

    uint64_t curr_epoch;

#if EXCLUSIVE_BATCH
    uint64_t idx = g_node_id;
    std::mutex clb_lock[MULTI_INSTANCES];
    uint64_t clb_cnt[MULTI_INSTANCES] = {0};

#endif

};

#endif
