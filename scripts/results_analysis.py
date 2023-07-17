#!/usr/bin/python
import numpy as np

DONE_TIMER=15
MULTI_INSTANCE=4
CLIENT_CNT=1
worker_thread_cnt = MULTI_INSTANCE + 3
Output_idle_times = list()
idle_times = list()
through_puts = list()
txn_cnts = list()
latencies = list()

def deal_with(out_str, i):
    # if out_str.find("idle_time_worker ") != -1:
    #     out_str = out_str[len("idle_time_worker "):]
    #     thd_id = out_str[:out_str.find("=")]
    #     idle_time = out_str[out_str.find("=") + 1 : -1]
    #     idle_times[int(thd_id)].append(float(idle_time))
    # elif out_str.find("Output ") != -1:
    #     idle_time = float(out_str[out_str.find(":") + 2: -1])
    #     Output_idle_times.append(idle_time)
    if out_str.find("tput         =") != -1:
        tput = float(out_str[out_str.find("=") + 1 : out_str.find("txn_cnt=")])
        through_puts[i] = tput
    elif out_str.find("txn_cnt=") == 0:
        txn_cnts.append(int(out_str[len("txn_cnt="):]))


def deal_with2(out_str, i):
    if out_str.find("AVG Latency: ") == 0:
        latencies.append(float(out_str[len("AVG Latency: "):]))


def print_max(i):
    worker_thread_idle_times = idle_times[i]
    result = "Max Idle Time for WorkerThread " + str(i) +" is " + \
             str(max(worker_thread_idle_times)) + " from " + \
             str(worker_thread_idle_times.index(max(worker_thread_idle_times))) + "\n"
    fw.write(result)


def print_min(i):
    worker_thread_idle_times = idle_times[i]
    result = "Min Idle Time for WorkerThread " + str(i) +" is " + \
             str(min(worker_thread_idle_times)) + " from " + \
             str(worker_thread_idle_times.index(min(worker_thread_idle_times))) + "\n"
    fw.write(result)

def print_average(i):
    worker_thread_idle_times = idle_times[i]
    result = "Average Idle Time for WorkerThread " + str(i) + " is " + str(np.mean(worker_thread_idle_times)) + "\n"
    fw.write(result)

def print_variation(i):
    worker_thread_idle_times = idle_times[i]
    result = "Variation of Idle Time for WorkerThread " + str(i) + " is " + str(np.var(worker_thread_idle_times)) + "\n"
    fw.write(result)

def print_total_worker_idle_time():
    sum_worker_idle_times = list()
    for i in range(MULTI_INSTANCE):
        value = 0
        for j in range(len(idle_times)):
            value += idle_times[j][i]
        sum_worker_idle_times.append(value)
    result = "The Max sum of the worker_idle_time is " + str(max(sum_worker_idle_times)) + "\n\n"
    fw.write(result)
    result = "The Min sum of the worker_idle_time is " + str(min(sum_worker_idle_times)) + "\n\n"
    fw.write(result)
    result = "The average sum of the worker_idle_time is " + str(np.mean(sum_worker_idle_times)) + "\n\n"
    fw.write(result)
    result = "The average sum of the non-worker_idle_time is " + str((MULTI_INSTANCE+3) * DONE_TIMER - np.mean(sum_worker_idle_times)) + "\n\n"
    fw.write(result)

    for i in range(MULTI_INSTANCE):
        sum_worker_idle_times[i] += Output_idle_times[i]
    result = "The Max sum of the worker_idle_time is " + str(max(sum_worker_idle_times)) + "\n\n"
    fw.write(result)
    result = "The Min sum of the worker_idle_time is " + str(min(sum_worker_idle_times)) + "\n\n"
    fw.write(result)
    result = "The average sum of idle_time is " + str(np.mean(sum_worker_idle_times)) + "\n\n"
    fw.write(result)
    result = "The average sum of non-idle_time is " + str((MULTI_INSTANCE+4) * DONE_TIMER - np.mean(sum_worker_idle_times)) + "\n\n"
    fw.write(result)

def print_output():
    result = "Max of Idle Time for Output Thread is " + str(max(Output_idle_times))\
             + " from " + str(Output_idle_times.index(max(Output_idle_times))) + "\n\n"
    fw.write(result)
    result = "Min of Idle Time for Output Thread is " + str(min(Output_idle_times)) \
             + " from " + str(Output_idle_times.index(min(Output_idle_times))) + "\n\n"
    fw.write(result)
    result = "Average of Idle Time for Output Thread is " + str(np.mean(Output_idle_times)) + "\n\n"
    fw.write(result)
    result = "Variation of Idle Time for Output Thread is " + str(np.var(Output_idle_times)) + "\n\n"
    fw.write(result)


def print_throughput():
    result = "Max of Throughput is " + str(max(through_puts)) \
             + " from " + str(through_puts.index(max(through_puts))) + "\n\n"
    fw.write(result)
    result = "Min of Throughput is " + str(min(through_puts)) \
             + " from " + str(through_puts.index(min(through_puts))) + "\n\n"
    fw.write(result)
    result = "Average of Throughput is " + str(np.mean(through_puts)) + "\n\n"
    fw.write(result)
    result = "Standard Variation of Throughput is " + str(np.std(through_puts)) + "\n\n"
    fw.write(result)


def print_txn_cnt():
    result = "Max of txn_cnt is " + str(max(txn_cnts)) \
             + " from " + str(txn_cnts.index(max(txn_cnts))) + "\n\n"
    fw.write(result)
    result = "Min of txn_cnt is " + str(min(txn_cnts)) \
             + " from " + str(txn_cnts.index(min(txn_cnts))) + "\n\n"
    fw.write(result)
    result = "Average of txn_cnt is " + str(np.mean(txn_cnts)) + "\n\n"
    fw.write(result)
    result = "Standard Variation of txn_cnt is " + str(np.std(txn_cnts)) + "\n\n"
    fw.write(result)


def print_latency():
    result = "Max of AVG Latency is " + str(max(latencies)) \
             + " from " + str(latencies.index(max(latencies)) + MULTI_INSTANCE) + "\n\n"
    fw.write(result)
    result = "Min of AVG Latency is " + str(min(latencies)) \
             + " from " + str(latencies.index(min(latencies)) + MULTI_INSTANCE) + "\n\n"
    fw.write(result)
    result = "Average of AVG Latency is " + str(np.mean(latencies)) + "\n\n"
    fw.write(result)
    result = "Standard Variation of AVG Latency is " + str(np.std(latencies)) + "\n\n"
    fw.write(result)


fw = open("results/oracle_pvp"+ str(MULTI_INSTANCE) + "_" + str(DONE_TIMER) +"s.out", "w+")

for i in range(worker_thread_cnt):
    idle_times.append(list())

for i in range(MULTI_INSTANCE):
    through_puts.append(0)
    fo = open("results/" + str(i) + ".out", "r+")
    while True:
        line = fo.readline()
        if not line:
            break
        deal_with(str(line), i)
    fo.close()

for i in range(MULTI_INSTANCE, MULTI_INSTANCE + CLIENT_CNT):
    fo = open("results/" + str(i) + ".out", "r+")
    while True:
        line = fo.readline()
        if not line:
            break
        deal_with2(str(line), i)
    fo.close()

# fw.write("==========WorkerThreads============\n")

# for i in range(worker_thread_cnt):
#     print_max(i)
#     print_min(i)
#     print_average(i)
#     print_variation(i)
#     fw.write("\n")

# fw.write("\n==========Total Idle_time==========\n")
# print_total_worker_idle_time()

# fw.write("\n==========OutputThread============\n")
# print_output()

fw.write("\n==========Throughput============\n")
print_throughput()

# fw.write("\n==========Txn_Cnt============\n")
# print_txn_cnt()

# fw.write("\n==========AVG_LATENCY============\n")
# print_latency()

fw.close()