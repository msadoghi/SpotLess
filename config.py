import sys


def update_config(node_cnt = 4, client_cnt = 4, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 10 * 10 ** 9, multi_instances = 1,
                  multi_threads=1, larger_txn = "false", extra_size = 0, pvp_fail = "false", rcc_fail = "false",
                  pbft_fail = "false", fail_divider = 128, new_divider = "false", enable_ask = "false",
                  dark_test = "false", equiv_test = "false", ignore_test = "false", fail_id = 2, dark_cnt = 0,
                  equiv_cnt = 0, ignore_cnt = 0, narwhal = "false"):
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
        "IGNORE_CNT": ignore_cnt,
        "NARWHAL": narwhal
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


    fo = open("./scripts/RunSystem.py")
    lines = fo.readlines()
    for i in range(len(lines)):
        if lines[i].startswith("nds="):
            lines[i] = "nds=" + str(node_cnt)
    fo.close()
    fo = open("./scripts/RunSystem.py", "w")
    for line in lines:
        fo.write(line)
    fo.close()


    fo = open("./scripts/scp_binaries.sh")
    lines = fo.readlines()
    for i in range(len(lines)):
        if lines[i].startswith("nodes="):
            lines[i] = "nodes=" + str(node_cnt)
    fo.close()
    fo = open("./scripts/scp_binaries.sh", "w")
    for line in lines:
        fo.write(line)
    fo.close()

experiment_name = sys.argv[1]
if experiment_name == "tput-hotstuff-4":
    update_config(node_cnt = 4, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "tput-hotstuff-16":
    update_config(node_cnt = 16, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "tput-hotstuff-32":
    update_config(node_cnt = 32, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "tput-hotstuff-64":
    update_config(node_cnt = 64, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "tput-hotstuff-96":
    update_config(node_cnt = 96, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "tput-hotstuff-128":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)

elif experiment_name == "batchsize-hotstuff-10":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 10, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "batchsize-hotstuff-50":
    update_config(node_cnt = 128, client_cnt = 16, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 50, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "batchsize-hotstuff-100":
    update_config(node_cnt = 128, client_cnt = 16, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "batchsize-hotstuff-200":
    update_config(node_cnt = 128, client_cnt = 16, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 200, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "batchsize-hotstuff-400":
    update_config(node_cnt = 128, client_cnt = 16, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 400, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)

elif experiment_name == "txnsize-hotstuff-48":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9)
elif experiment_name == "txnsize-hotstuff-200":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9,
                  larger_txn="true", extra_size=152)
elif experiment_name == "txnsize-hotstuff-400":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9,
                  larger_txn="true", extra_size=352)
elif experiment_name == "txnsize-hotstuff-800":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9,
                  larger_txn="true", extra_size=752)
elif experiment_name == "txnsize-hotstuff-1600":
    update_config(node_cnt = 128, client_cnt = 1, input_thread = 4, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 30 * 10 ** 9,
                  larger_txn="true", extra_size=1552)

elif experiment_name == "tput-lat-hotstuff-p1":
    update_config(node_cnt=128, client_cnt=1, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, max_txn_inflight=80000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9)

elif experiment_name == "tput-lat-hotstuff-p1":
    update_config(node_cnt=128, client_cnt=1, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, max_txn_inflight=80000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9)
elif experiment_name == "tput-lat-hotstuff-p2":
    update_config(node_cnt=128, client_cnt=1, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9)
elif experiment_name == "tput-lat-hotstuff-p3":
    update_config(node_cnt=128, client_cnt=1, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, max_txn_inflight=20000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9)
elif experiment_name == "tput-lat-hotstuff-p4":
    update_config(node_cnt=128, client_cnt=1, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, max_txn_inflight=10000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9)
elif experiment_name == "tput-lat-hotstuff-p5":
    update_config(node_cnt=128, client_cnt=1, input_thread=8, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, max_txn_inflight=5000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9)


