#include "msg_thread.h"
#include "msg_queue.h"
#include "message.h"
#include "mem_alloc.h"
#include "transport.h"
#include "query.h"
#include "ycsb_query.h"
#include "pool.h"
#include "global.h"
#include "crypto.h"

void MessageThread::init(uint64_t thd_id)
{
    buffer_cnt = g_total_node_cnt;
    DEBUG_M("MessageThread::init buffer[] alloc\n");
    buffer = (mbuf **)mem_allocator.align_alloc(sizeof(mbuf *) * buffer_cnt);
    for (uint64_t n = 0; n < buffer_cnt; n++)
    {
        DEBUG_M("MessageThread::init mbuf alloc\n");
        buffer[n] = (mbuf *)mem_allocator.align_alloc(sizeof(mbuf));
        buffer[n]->init(n);
        buffer[n]->reset(n);
    }
    _thd_id = thd_id;
}

void MessageThread::check_and_send_batches()
{
    for (uint64_t dest_node_id = 0; dest_node_id < buffer_cnt; dest_node_id++)
    {
        if (buffer[dest_node_id]->ready())
        {
            send_batch(dest_node_id);
        }
    }
}

void MessageThread::send_batch(uint64_t dest_node_id)
{
    mbuf *sbuf = buffer[dest_node_id];
    assert(sbuf->cnt > 0);
    ((uint32_t *)sbuf->buffer)[2] = sbuf->cnt;
    //printf("Send batch of %ld msgs to %ld\n", sbuf->cnt, dest_node_id);
    DEBUG("Send batch of %ld msgs to %ld\n", sbuf->cnt, dest_node_id);
    tport_man.send_msg(_thd_id, dest_node_id, sbuf->buffer, sbuf->ptr);
    sbuf->reset(dest_node_id);
}

void MessageThread::run()
{
    Message *msg = NULL;
    uint64_t dest_node_id;
    vector<uint64_t> dest;
    vector<string> allsign;
    mbuf *sbuf;

    // Relative Id of the server's output thread.
#if TRANSPORT_OPTIMIZATION
    UInt32 td_id = 0;
    if(ISSERVER) 
        td_id = (_thd_id - g_thread_cnt - g_this_rem_thread_cnt) % g_this_send_thread_cnt;
    else
        td_id = (_thd_id - g_client_thread_cnt - g_this_rem_thread_cnt) % g_this_send_thread_cnt;
#else
    UInt32 td_id = _thd_id % g_this_send_thread_cnt;
#endif

#if SEMA_TEST
    if(ISSERVER){
        if (simulation->is_warmup_done()){
            idle_starttime = get_sys_clock();
        }
        // Wait until there is a msg in the queue (the value of the semaphore is not zero), then decrease the value by 1
        sem_wait(&output_semaphore[td_id]);
        if (idle_starttime > 0 && simulation->is_warmup_done()){
            output_thd_idle_time[td_id] += get_sys_clock() - idle_starttime;
        }    
    } 
#endif

    // dest = msg_queue.dequeue(get_thd_id(), allsign, msg);
    msg_queue.dequeue(get_thd_id(), allsign, msg);
    if (!msg)
    {
        check_and_send_batches();
        if (idle_starttime == 0)
        {
            idle_starttime = get_sys_clock();
        }
        return;
    }
#if !SEMA_TEST
    if (idle_starttime > 0 && simulation->is_warmup_done())
    {
        output_thd_idle_time[td_id] += get_sys_clock() - idle_starttime;
        idle_starttime = 0;
    }
#endif
    assert(msg);

    // for (uint64_t i = 0; i < dest.size(); i++)
    for (uint64_t i = 0; i < msg->dest.size(); i++)
    {
        dest_node_id = msg->dest[i];

        if (ISSERVER)
        {
            if (dest_node_id % g_this_send_thread_cnt != td_id)
            {
                continue;
            }
        }
        // Adding signature, if present.
        if (allsign.size() > 0)
        {
            msg->signature = allsign[i];
            switch (msg->rtype)
            {
            case CL_BATCH:
                msg->pubKey = getOtherRequiredKey(dest_node_id);
                break;

            default:
                msg->pubKey = getCmacRequiredKey(dest_node_id);
            }
            msg->sigSize = msg->signature.size();
            msg->keySize = msg->pubKey.size();
        }
        sbuf = buffer[dest_node_id];
        if (!sbuf->fits(msg->get_size()))
        {
            assert(sbuf->cnt > 0);
            cout << "not fitting " << sbuf->cnt << endl;
            send_batch(dest_node_id);
        }
        if (msg->rtype == PBFT_CHKPT_MSG){
            sbuf->force = true;
        }
        else if(msg->rtype == PVP_GENERIC_MSG){
             sbuf->force = true;
        }else if(msg->force == true){
            sbuf->force = true;
        }
        msg->copy_to_buf(&(sbuf->buffer[sbuf->ptr]));
        sbuf->cnt += 1;
        sbuf->ptr += msg->get_size();

        if (sbuf->starttime == 0)
            sbuf->starttime = get_sys_clock();
        check_and_send_batches();
    }
    Message::release_message(msg);
}
