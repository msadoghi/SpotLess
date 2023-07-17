import sys


def update_config(node_cnt = 4, client_cnt = 4, input_thread = 3, output_thread = 4, client_input_thread = 12,
                  client_output_thread = 1, client_thread = 1, message_per_buffer = 1, max_txn_inflight = 40000,
                  batch_size = 100, done_timer = 120 * 10 ** 9, warmup_timer = 10 * 10 ** 9, multi_instances = 1,
                  multi_threads=1, larger_txn = "false", extra_size = 0, pvp_fail = "false", rcc_fail = "false",
                  pbft_fail = "false", fail_divider = 128, new_divider = "false", enable_ask = "false",
                  dark_test = "false", equiv_test = "false", ignore_test = "false", fail_id = 2, dark_cnt = 0,
                  equiv_cnt = 0, ignore_cnt = 0, narwhal = "false", hs_fail = "false"):
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
        "SpotLess_FAIL": pvp_fail,
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
        "NARWHAL": narwhal,
        "HS_FAIL": hs_fail,
        "CRASH_DIVIDER": fail_divider,
        "CRASH_ID": fail_id,
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
if experiment_name == "failure-hotstuff-128-1":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=128)
elif experiment_name == "failure-hotstuff-128-2":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=64)
elif experiment_name == "failure-hotstuff-128-3":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=43)
elif experiment_name == "failure-hotstuff-128-4":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=32)
elif experiment_name == "failure-hotstuff-128-6":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=22)
elif experiment_name == "failure-hotstuff-128-8":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=16)
elif experiment_name == "failure-hotstuff-128-10":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=13)
elif experiment_name == "failure-hotstuff-128-40f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=8)
elif experiment_name == "failure-hotstuff-128-60f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=5, fail_id=4)
elif experiment_name == "failure-hotstuff-128-80f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=4)
elif experiment_name == "failure-hotstuff-128-100f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", new_divider="true")

elif experiment_name == "failure-narwhal-128-1":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=128, narwhal="true")
elif experiment_name == "failure-narwhal-128-2":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=64, narwhal="true")
elif experiment_name == "failure-narwhal-128-3":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=96, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=43, narwhal="true")
elif experiment_name == "failure-narwhal-128-4":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=32, narwhal="true")
elif experiment_name == "failure-narwhal-128-6":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=22, narwhal="true")
elif experiment_name == "failure-narwhal-128-8":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=16, narwhal="true")
elif experiment_name == "failure-narwhal-128-10":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=72, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=13, narwhal="true")
elif experiment_name == "failure-narwhal-128-40f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=8, narwhal="true")
elif experiment_name == "failure-narwhal-128-60f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=5, fail_id=4, narwhal="true")
elif experiment_name == "failure-narwhal-128-80f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=64, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", fail_divider=4, narwhal="true")
elif experiment_name == "failure-narwhal-128-100f":
    update_config(node_cnt=128, client_cnt=1, input_thread=4, output_thread=4, client_input_thread=12,
                  client_output_thread=1, client_thread=1, message_per_buffer=48, max_txn_inflight=40000,
                  batch_size=100, done_timer=120 * 10 ** 9, warmup_timer=30 * 10 ** 9, multi_instances=1,
                  multi_threads=1, hs_fail="true", new_divider="true", narwhal="true")
