import sys


def update_config(node_cnt = 4, client_cnt = 4, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 10 * 10 ** 9, multi_instances = 4,
                  multi_threads = 4, larger_txn = "false", extra_size = 0, pvp_fail = "false", rcc_fail = "false",
                  pbft_fail = "false", fail_divider = 128, new_divider = "false", enable_ask = "false",
                  dark_test = "false", equiv_test = "false", ignore_test = "false", fail_id = 2, dark_cnt = 0,
                  equiv_cnt = 0, ignore_cnt = 0):
    filename = "./config.h"
    fo = open(filename)
    lines = fo.readlines()
    dic = {
        "NODE_CNT": node_cnt,
        "CLIENT_NODE_CNT": client_cnt,
        "REM_THREAD_CNT": input_thread,
        "SEND_THREAD_CNT": output_thread,
        "CLIENT_THREAD_CNT": client_thread,
        "CLIENT_REM_THREAD_CNT": client_input_thread,
        "CLIENT_SEND_THREAD_CNT": client_output_thread,
        "MESSAGE_PER_BUFFER": message_per_buffer,
        "MAX_TXN_INFLIGHT": max_txn_inflight,
        "BATCH_SIZE": batch_size,
        "DONE_TIMER": done_timer,
        "WARMUP_TIMER": warmup_timer,
        "MULTI_INSTANCES": multi_instances,
        "MULTI_THREADS": multi_threads,
        "LARGER_TXN": larger_txn,
        "EXTRA_SIZE": extra_size,
        "PVP_FAIL": pvp_fail,
        "RCC_FAIL": rcc_fail,
        "PBFT_FAIL": pbft_fail,
        "FAIL_DIVIDER": fail_divider,
        "NEW_DIVIDER": new_divider,
        "ENABLE_ASK": enable_ask,
        "DARK_TEST": dark_test,
        "EQUIV_TEST": equiv_test,
        "IGNORE_TEST": ignore_test,
        "FAIL_ID": fail_id,
        "EQUIV_CNT": equiv_cnt,
        "DARK_CNT": dark_cnt,
        "IGNORE_CNT": ignore_cnt
    }
    for key, value in dic.items():
        key_str = "#define " + key + " "
        for i in range(len(lines)):
            if lines[i].find(key_str) == 0:
                lines[i] = key_str + str(value) + "\n"
    for i in range(len(lines)):
        print(lines[i])
    fo.close()
    fo = open(filename, "w")
    for line in lines:
        fo.write(line)
    fo.close()


experiment_name = sys.argv[1]
if experiment_name == "tput-pvp-4":
    update_config(node_cnt = 4, client_cnt = 4, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 3, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 4,
                  multi_threads = 4)
elif experiment_name == "tput-pvp-16":
    update_config(node_cnt = 16, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 12, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 16,
                  multi_threads = 16)
elif experiment_name == "tput-pvp-32":
    update_config(node_cnt = 32, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 24, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 32,
                  multi_threads = 32)
elif experiment_name == "tput-pvp-64":
    update_config(node_cnt = 64, client_cnt = 32, input_thread = 5, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 64, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 64,
                  multi_threads = 64)
elif experiment_name == "tput-pvp-96":
    update_config(node_cnt = 96, client_cnt = 32, input_thread = 7, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 96, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 96,
                  multi_threads = 96)
elif experiment_name == "tput-pvp-128":
    update_config(node_cnt = 128, client_cnt = 32, input_thread = 8, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 128, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 128,
                  multi_threads = 128)

elif experiment_name == "batchsize-pvp-10":
    update_config(node_cnt = 32, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 24, max_txn_inflight = 40000,
                  batch_size = 10, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 32,
                  multi_threads = 32)
elif experiment_name == "batchsize-pvp-50":
    update_config(node_cnt = 32, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 24, max_txn_inflight = 40000,
                  batch_size = 50, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 32,
                  multi_threads = 32)
elif experiment_name == "batchsize-pvp-100":
    update_config(node_cnt = 32, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 24, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 32,
                  multi_threads = 32)
elif experiment_name == "batchsize-pvp-200":
    update_config(node_cnt = 32, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 24, max_txn_inflight = 40000,
                  batch_size = 200, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 32,
                  multi_threads = 32)
elif experiment_name == "batchsize-pvp-400":
    update_config(node_cnt = 32, client_cnt = 16, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 24, max_txn_inflight = 40000,
                  batch_size = 400, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 32,
                  multi_threads = 32)

elif experiment_name == "txnsize-pvp-48":
    update_config(node_cnt = 128, client_cnt = 32, input_thread = 8, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 128, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 128,
                  multi_threads = 128)
elif experiment_name == "txnsize-pvp-200":
    update_config(node_cnt = 128, client_cnt = 32, input_thread = 8, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 128, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 128,
                  multi_threads = 128, larger_txn="true", extra_size=152)
elif experiment_name == "txnsize-pvp-400":
    update_config(node_cnt = 128, client_cnt = 32, input_thread = 8, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 128, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 128,
                  multi_threads = 128, larger_txn="true", extra_size=352)
elif experiment_name == "txnsize-pvp-800":
    update_config(node_cnt = 128, client_cnt = 32, input_thread = 8, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 128, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 128,
                  multi_threads = 128, larger_txn="true", extra_size=752)
elif experiment_name == "txnsize-pvp-1600":
    update_config(node_cnt = 128, client_cnt = 32, input_thread = 8, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 128, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9, multi_instances = 128,
                  multi_threads = 128, larger_txn="true", extra_size=1552)

elif experiment_name == "tput-lat-pvp-p1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=80000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128)

elif experiment_name == "tput-lat-pvp-p1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=80000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128)
elif experiment_name == "tput-lat-pvp-p2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128)
elif experiment_name == "tput-lat-pvp-p3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=20000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128)
elif experiment_name == "tput-lat-pvp-p4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=32, max_txn_inflight=10000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128)
elif experiment_name == "tput-lat-pvp-p5":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=16, max_txn_inflight=5000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128)

elif experiment_name == "failure-pvp-32-1":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=24, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", fail_divider=32)
elif experiment_name == "failure-pvp-32-2" or experiment_name == "failure-pvp-32-20f":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=24, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", fail_divider=16)
elif experiment_name == "failure-pvp-32-3":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=24, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", fail_divider=11)
elif experiment_name == "failure-pvp-32-4" or experiment_name == "failure-pvp-32-40f":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=16, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", fail_divider=8)
elif experiment_name == "failure-pvp-32-6" or experiment_name == "failure-pvp-32-60f":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=16, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", fail_divider=6)
elif experiment_name == "failure-pvp-32-8" or experiment_name == "failure-pvp-32-80f":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=16, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", fail_divider=4)
elif experiment_name == "failure-pvp-32-10" or experiment_name == "failure-pvp-32-100f":
    update_config(node_cnt=32, client_cnt=16, input_thread=3, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=12, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=32,
                  multi_threads=32, pvp_fail="true", new_divider="true")

elif experiment_name == "failure-pvp-64-1":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=64)
elif experiment_name == "failure-pvp-64-2":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=32)
elif experiment_name == "failure-pvp-64-3":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=22)
elif experiment_name == "failure-pvp-64-4" or experiment_name == "failure-pvp-64-20f":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=40, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=16)
elif experiment_name == "failure-pvp-64-6":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=40, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=11)
elif experiment_name == "failure-pvp-64-8" or experiment_name == "failure-pvp-64-40f":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=40, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=8)
elif experiment_name == "failure-pvp-64-10":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=32, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=6, fail_id=4)
elif experiment_name == "failure-pvp-64-60f":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=32, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=5)
elif experiment_name == "failure-pvp-64-80f":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=32, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", fail_divider=4)
elif experiment_name == "failure-pvp-64-100f":
    update_config(node_cnt=64, client_cnt=32, input_thread=5, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=24, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=64,
                  multi_threads=64, pvp_fail="true", new_divider="true")


elif experiment_name == "failure-pvp-96-1":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=96)
elif experiment_name == "failure-pvp-96-2":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=48)
elif experiment_name == "failure-pvp-96-3":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=32)
elif experiment_name == "failure-pvp-96-4":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=24)
elif experiment_name == "failure-pvp-96-6" or experiment_name == "failure-pvp-96-20f":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=16)
elif experiment_name == "failure-pvp-96-8":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=12)
elif experiment_name == "failure-pvp-96-10":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=10)
elif experiment_name == "failure-pvp-96-40f":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=8)
elif experiment_name == "failure-pvp-96-60f":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=5)
elif experiment_name == "failure-pvp-96-80f":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", fail_divider=4)
elif experiment_name == "failure-pvp-96-100f":
    update_config(node_cnt=96, client_cnt=32, input_thread=7, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=96,
                  multi_threads=96, pvp_fail="true", new_divider="true")

elif experiment_name == "failure-pvp-128-1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=128)
elif experiment_name == "failure-pvp-128-2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=64)
elif experiment_name == "failure-pvp-128-3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=43)
elif experiment_name == "failure-pvp-128-4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=32)
elif experiment_name == "failure-pvp-128-6":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=22)
elif experiment_name == "failure-pvp-128-8" or experiment_name == "failure-pvp-128-20f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=16)
elif experiment_name == "failure-pvp-128-10":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=13)
elif experiment_name == "failure-pvp-128-40f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=8)
elif experiment_name == "failure-pvp-128-60f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=5, fail_id=4)
elif experiment_name == "failure-pvp-128-80f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=4)
elif experiment_name == "failure-pvp-128-100f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", new_divider="true")

elif experiment_name == "tput-lat-failure-pvp-1-p1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=80000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=128)
elif experiment_name == "tput-lat-failure-pvp-1-p2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=128)
elif experiment_name == "tput-lat-failure-pvp-1-p3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=20000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=128)
elif experiment_name == "tput-lat-failure-pvp-1-p4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=32, max_txn_inflight=10000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=128)
elif experiment_name == "tput-lat-failure-pvp-1-p5":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=16, max_txn_inflight=5000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", fail_divider=128)

elif experiment_name == "tput-lat-failure-pvp-f-p1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=40, max_txn_inflight=80000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", new_divider="true")
elif experiment_name == "tput-lat-failure-pvp-f-p2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=40, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", new_divider="true")
elif experiment_name == "tput-lat-failure-pvp-f-p3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=32, max_txn_inflight=20000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", new_divider="true")
elif experiment_name == "tput-lat-failure-pvp-f-p4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=24, max_txn_inflight=15000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", new_divider="true")
elif experiment_name == "tput-lat-failure-pvp-f-p5":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=14, max_txn_inflight=9200,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, pvp_fail="true", new_divider="true")


elif experiment_name == "A2-pvp-128-1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=1)
elif experiment_name == "A2-pvp-128-2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=2)
elif experiment_name == "A2-pvp-128-3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=3)
elif experiment_name == "A2-pvp-128-4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=4)
elif experiment_name == "A2-pvp-128-6":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=6)
elif experiment_name == "A2-pvp-128-8" or experiment_name == "failure-pvp-128-20f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=8)
elif experiment_name == "A2-pvp-128-10":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=10)
elif experiment_name == "A2-pvp-128-40f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=16)
elif experiment_name == "A2-pvp-128-60f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=25)
elif experiment_name == "A2-pvp-128-80f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=33)
elif experiment_name == "A2-pvp-128-100f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", dark_test="true", dark_cnt=42)


elif experiment_name == "A3-pvp-128-1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=1)
elif experiment_name == "A3-pvp-128-2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=2)
elif experiment_name == "A3-pvp-128-3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=3)
elif experiment_name == "A3-pvp-128-4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=4)
elif experiment_name == "A3-pvp-128-6":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=6)
elif experiment_name == "A3-pvp-128-8" or experiment_name == "failure-pvp-128-20f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=8)
elif experiment_name == "A3-pvp-128-10":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=10)
elif experiment_name == "A3-pvp-128-40f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=16)
elif experiment_name == "A3-pvp-128-60f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=25)
elif experiment_name == "A3-pvp-128-80f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=33)
elif experiment_name == "A3-pvp-128-100f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, enable_ask="true", equiv_test="true", equiv_cnt=42)


elif experiment_name == "A4-pvp-128-1":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=1)
elif experiment_name == "A4-pvp-128-2":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=2)
elif experiment_name == "A4-pvp-128-3":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=3)
elif experiment_name == "A4-pvp-128-4":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=4)
elif experiment_name == "A4-pvp-128-6":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=6)
elif experiment_name == "A4-pvp-128-8" or experiment_name == "failure-pvp-128-20f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=8)
elif experiment_name == "A4-pvp-128-10":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=10)
elif experiment_name == "A4-pvp-128-40f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=16)
elif experiment_name == "A4-pvp-128-60f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=25)
elif experiment_name == "A4-pvp-128-80f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=33)
elif experiment_name == "A4-pvp-128-100f":
    update_config(node_cnt=128, client_cnt=32, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=128,
                  multi_threads=128, ignore_test="true", ignore_cnt=42)

