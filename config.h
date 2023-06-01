#ifndef _CONFIG_H_
#define _CONFIG_H_
// Specify the number of servers or replicas
#define NODE_CNT 128

// make clean; make -j8; python3 scripts/ifconfig.py
// python3 scripts/StopSystem.py; python3 scripts/scp_binaries.py; python3 scripts/RunSystem.py
// python3 scripts/StopSystem.py; python3 scripts/scp_results.py
// python3 scripts/results_analysis.py
// ssh -i oracle2.key ubuntu@10.0.112.243
// Number of worker threads at primary. For RBFT (6) and other algorithms (5). For PVP (NODE_CNT + 3).
#define THREAD_CNT (PROPOSAL_THREAD? MULTI_THREADS+5: MULTI_INSTANCES+3)
#define REM_THREAD_CNT 8
#define SEND_THREAD_CNT 4
#define PART_CNT 1
// Specify the number of clients.
#define CLIENT_NODE_CNT 8
#define CLIENT_THREAD_CNT 1
#define CLIENT_REM_THREAD_CNT 12
#define CLIENT_SEND_THREAD_CNT 1
#define CLIENT_RUNTIME false

#define MESSAGE_PER_BUFFER 4

#define LOAD_PER_SERVER 1
#define REPLICA_CNT 0
#define REPL_TYPE AP
#define VIRTUAL_PART_CNT PART_CNT
#define PAGE_SIZE 4096
#define CL_SIZE 64
#define CPU_FREQ 2.2
#define HW_MIGRATE false
#define WARMUP 0
#define WORKLOAD YCSB
#define PRT_LAT_DISTR false
#define STATS_ENABLE true
#define STATS_DETAILED false
#define STAT_BAND_WIDTH_ENABLE false
#define TIME_ENABLE true
#define TIME_PROF_ENABLE false
#define FIN_BY_TIME true
// Number of transactions each client should send without waiting.
#define MAX_TXN_IN_FLIGHT (160*BATCH_SIZE)
#define SERVER_GENERATE_QUERIES false
#define MEM_ALLIGN 8
#define THREAD_ALLOC false
#define THREAD_ARENA_SIZE (1UL << 22)
#define MEM_PAD true
#define PART_ALLOC false
#define MEM_SIZE (1UL << 30)
#define NO_FREE false
#define TPORT_TYPE TCP
#define TPORT_PORT 10000
#define TPORT_WINDOW 20000
#define SET_AFFINITY false
#define MAX_TPORT_NAME 128
#define MSG_SIZE 128
#define HEADER_SIZE sizeof(uint32_t) * 2
#define MSG_TIMEOUT 5000000000UL // in ns
#define NETWORK_TEST false
#define NETWORK_DELAY_TEST false
#define NETWORK_DELAY 0UL
#define MAX_QUEUE_LEN NODE_CNT * 2
#define PRIORITY_WORK_QUEUE false
#define PRIORITY PRIORITY_ACTIVE
#define MSG_SIZE_MAX 1048576
#define MSG_TIME_LIMIT 0
#define KEY_ORDER false
#define ENABLE_LATCH false
#define CENTRAL_INDEX false
#define CENTRAL_MANAGER false
#define INDEX_STRUCT IDX_HASH
#define BTREE_ORDER 16
#define TS_TWR false
#define TS_ALLOC TS_CLOCK
#define TS_BATCH_ALLOC false
#define TS_BATCH_NUM 1
#define HIS_RECYCLE_LEN 10
#define MAX_PRE_REQ MAX_TXN_IN_FLIGHT *NODE_CNT
#define MAX_READ_REQ MAX_TXN_IN_FLIGHT *NODE_CNT
#define MIN_TS_INTVL 10 * 1000000UL
#define MAX_WRITE_SET 10
#define PER_ROW_VALID false
#define TXN_QUEUE_SIZE_LIMIT THREAD_CNT
#define SEQ_THREAD_CNT 4
#define MAX_ROW_PER_TXN 64
#define QUERY_INTVL 1UL
#define MAX_TXN_PER_PART 4000
#define FIRST_PART_LOCAL true
#define MAX_TUPLE_SIZE 1024
#define GEN_BY_MPR false
#define SKEW_METHOD ZIPF
#define DATA_PERC 100
#define ACCESS_PERC 0.03
#define INIT_PARALLELISM 8
#define SYNTH_TABLE_SIZE 524288

#define ZIPF_THETA 0.5
#define WRITE_PERC 0.9
#define TXN_WRITE_PERC 0.5
#define TUP_WRITE_PERC 0.5
#define SCAN_PERC 0
#define SCAN_LEN 20
#define PART_PER_TXN PART_CNT
#define PERC_MULTI_PART MPR
#define REQ_PER_QUERY 1
#define FIELD_PER_TUPLE 10
#define CREATE_TXN_FILE false
#define SINGLE_THREAD_WL_GEN true
#define STRICT_PPT 1
#define MPR 1.0
#define MPIR 0.01
#define WL_VERB true
#define IDX_VERB false
#define VERB_ALLOC true
#define DEBUG_LOCK false
#define DEBUG_TIMESTAMP false
#define DEBUG_SYNTH false
#define DEBUG_ASSERT false
#define DEBUG_DISTR false
#define DEBUG_ALLOC false
#define DEBUG_RACE false
#define DEBUG_TIMELINE false
#define DEBUG_BREAKDOWN false
#define DEBUG_LATENCY false
#define DEBUG_QUECC false
#define DEBUG_WLOAD false
#define MODE NORMAL_MODE
#define DBTYPE REPLICATED
#define IDX_HASH 1
#define IDX_BTREE 2
#define YCSB 1
#define TEST 4
#define TS_MUTEX 1
#define TS_CAS 2
#define TS_HW 3
#define TS_CLOCK 4
#define ZIPF 1
#define HOT 2
#define PRIORITY_FCFS 1
#define PRIORITY_ACTIVE 2
#define PRIORITY_HOME 3
#define AA1 1
#define AP 2
#define LOAD_MAX 1
#define LOAD_RATE 2
#define TCP 1
#define IPC 2
#define BILLION 1000000000UL
#define MILLION 1000000UL
#define STAT_ARR_SIZE 1024
#define PROG_TIMER 1 * BILLION
#define SEQ_BATCH_TIMER 5 * 1 * MILLION
#define SEED 0
#define SHMEM_ENV false
#define ENVIRONMENT_EC2 false
#define PARTITIONED 0
#define REPLICATED 1
// To select the amount of time to warmup and run.
#define DONE_TIMER 120 * BILLION
#define WARMUP_TIMER  3 * BILLION
// Select the consensus algorithm to run.
#define CONSENSUS HOTSTUFF
#define DBFT 1
#define PBFT 2
#define ZYZZYVA 3
#define HOTSTUFF 4
// Size of each batch.
#define LARGER_TXN false
#define EXTRA_SIZE 0
#define BATCH_SIZE 100
#define BATCH_ENABLE true
// Number of transactions to wait for period checkpointing.
#define TXN_PER_CHKPT NODE_CNT * BATCH_SIZE * 32
#define EXECUTION_THREAD true
#define EXECUTE_THD_CNT 1
#define CLIENT_BATCH true
#define CLIENT_RESPONSE_BATCH true
// To Enable or disable the blockchain implementation.
#define ENABLE_CHAIN false
//Global variables to choose the encryptation algorithm
#define USE_CRYPTO true
#define CRYPTO_METHOD_RSA false     //Options RSA,
#define CRYPTO_METHOD_ED25519 true  // Option ED25519
#define CRYPTO_METHOD_CMAC_AES true // CMAC<AES>

// To allow testing in-memory database or SQLite.
// Further, using SQLite a user can also choose to persist the data.
#define EXT_DB MEMORY
#define MEMORY 1
#define SQL 2
#define SQL_PERSISTENT 3

// To allow testing of a Banking Smart Contracts.
#define BANKING_SMART_CONTRACT false

// Switching on MultiBFT or PVP
#define MULTI_THREADS (MULTI_INSTANCES > 16 ? 16 : MULTI_INSTANCES)
#define MULTI_INSTANCES NODE_CNT

// Entities for debugging
#define SEND_NEWVIEW_PRINT PROCESS_PRINT
#define PROCESS_PRINT false
#define PRINT_KEYEX false
#define SEMA_TEST true

#define FIX_INPUT_THREAD_BUG true
#define FIX_CL_INPUT_THREAD_BUG true
#define TRANSPORT_OPTIMIZATION true

#define AUTO_POST PVP_FAIL
#define TIMER_MANAGER PVP_FAIL
#define PVP_FAIL true

#define THRESHOLD_SIGNATURE true
#define SECP256K1 true
#define ENABLE_ENCRYPT true
#define ENABLE_EXECUTE true

#define FIX_MEM_LEAK true

#define TEMP_QUEUE true
#define MAC_SYNC true
#define SYNC_QC true
#define SEPARATE true
#define ROUNDS_IN_ADVANCE 0
#define PROPOSAL_THREAD true
#define MAC_VERSION true

#define INITIAL_TIMEOUT_LENGTH 1*BILLION

#define CRASH_VIEW 100

#define EXCLUSIVE_BATCH true
#define FAIL_DIVIDER 3
#define FAIL_ID 2
#define MAX_TIMER_LEN 200000000

#define NEW_DIVIDER true
#define DIV1 6
#define DIV2 3
#define LIMIT1 3
#define LIMIT2 1

#define ENABLE_ASK false
#define DARK_TEST false
#define DARK_ID 2
#define VICTIM_ID 1
#define DARK_FREQ 3
#define DARK_CNT 42

#define IGNORE_TEST false
#define IGNORE_ID 2
#define IGNORE_FREQ 3
#define IGNORE_CNT 42


#define EQUIV_TEST false
#define EQUIV_FREQ 3
#define EQUIV_ID 2
#define EQ_VICTIM_ID 1
#define EQUIV_CNT 1

#endif
