# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

### ResilientDB aims at *Making Permissioned Blockchain Systems Fast Again*. ResilientDB makes *system-centric* design decisions by adopting a *multi-thread architecture* that encompasses *deep-pipelines*. Further, we *separate* the ordering of client transactions from their execution, which allows us to perform *out-of-order processing of messages*.

### Quick Facts about Version 2.0 of ResilientDB

1. Consensus protocols like **PBFT, RCC, HotStuff and PVP** are implemented in ResilientDB
2. ResilientDB expects minimum **3f+1** replicas, where **f** is the maximum number of byzantine (or malicious) replicas.
3. At present, each client only sends YCSB-style transactions for processing, to the primary.
4. Each client transaction has an associated **transaction manager**, which stores all the data related to the transaction.
5. Depending on the type of replica (primary or non-primary), we associate different a number of threads and queues with each replica.
6. To facilitate data storage and persistence, ResilientDB provides support for an **in-memory key-value store**. 

---

## Steps to Run and Compile<br/>

* **ResileintDB is only supported on Ubuntu, and we recommend Ubuntu 20.04**

* The first step is to untar the dependencies:

  ​    cd deps && \ls | xargs -i tar -xvf {} && cd ..

* Create **obj** folder inside **resilientdb** folder, to store object files. And **results** to store the results.

  ​    mkdir obj
  ​    mkdir results

* Collect the IP addresses of the machines that you will run resilientDB and put them into **scripts/hostnames.py**

* Below is the configuration of the machines on which we conducted our experiments

  * Oracle Cloud
  * VM.Standard.E4.Flex
  * 16-core AMD EPYC 7742 processor running at 3.4 GHz
  * 32 GB memory
  * Ubuntu 20.04
  * Networking Configuration with TCP connections allowed

* Search "IDENTITY" in this repository, and replace all of them with the SSH private key file for the machines on which you will 
  run resilientDB. (All the machines use the same SSH key pair).

  ​      IDENTITY="your_SSH_private_key_file"

* Deploy the necessary environment to run resilientDB on the machines

  ​    python3 scripts/nodeModify.py

* Select the machines that you will run resilientDB in the next experiment in **scripts/hostnames.py**. For example, you will run resilientDB with 4 replicas and 1 client, choosing the first 8 machines.

  ​    hostip_machines = hostip_phx[:5]

* Generate **ifconfig.txt**

* Here are important relevant parameters of "config.h"

<pre>
* NODE_CNT                      Total number of replicas, minimum 4, that is, f=1.  
* THREAD_CNT                    Total number of threads at primary
* REM_THREAD_CNT                Total number of input threads at a replica 
* SEND_THREAD_CNT               Total number of output threads at a replica
* CLIENT_NODE_CNT               Total number of clients
* CLIENT_THREAD_CNT             Total number of threads at a client
* CLIENT_REM_THREAD_CNT         Total number of input threads at a client
* CLIENT_SEND_THREAD_CNT        Total number of output threads at a client
* MAX_TXN_IN_FLIGHT             Number of inflight transactions that a client can have, which are sent but not responded 
* DONE_TIMER                    Amount of time to run the system.
* WARMUP_TIMER                  Amount of time to warmup the system (No statistics collected).
* BATCH_SIZE                    Number of transactions in a batch (at least 5)
* TXN_PER_CHKPT                 Frequency at which garbage collection is done.
* MESSAGE_PER_BUFFER            The number of messages that a replica sends at one time
* MULTI_INSTANCES								The number of concurrent instances
* ...
</pre>


* For example, to run PVP with 4 replicas and 1 client (the current setting in the *main* branch), we set the parameters as follows (Keep other parameters unchanged).

  | PARAMETER_NAME         | VALUE            |
  | ---------------------- | ---------------- |
  | NODE_CNT               | 4                |
  | THREAD_CNT             | MULTI_THREADS+5  |
  | REM_THREAD_CNT         | 3                |
  | SEND_THREAD_CNT        | 4                |
  | CLIENT_NODE_CNT        | 1                |
  | CLIENT_THREAD_CNT      | 1                |
  | CLIENT_REM_THREAD_CNT  | 12               |
  | CLINET_SEND_THREAD_CNT | 1                |
  | MAX_TXN_IN_FLIGHT      | (400*BATCH_SIZE) |
  | DONE_TIMER             | 120 * BILLION    |
  | WARMUP_TIMER           | 10 * BILLION     |
  | BATCH_SIZE             | 100              |
  | MESSAGE_PER_BUFFER     | 3                |
  | MULTI_INSTANCES        | NODE_CNT         |


* Compile the code. On compilation, two new files are created: **runcl** and **rundb**. You may fail to compile due to the lack of some packages. Please install them following the Error information.
        
      make clean; make -j8;

* Configure the replica number parameters in scripts


  * ./scripts    nodes=4
  * ./scripts/RunSystem.py nds=4

* Copy the **rundb** to the 4 replicas and **runcl** to the 1 client, and run resilientDB
        
      python3 scripts/StopSystem.py; python3 scripts/scp_binaries.py; python3 scripts/RunSystem.py

* Collect Results.

  ​    python3 scripts/scp_results.py


* Note: We specify the parameter setup of different experiments in our paper in this form (https://docs.google.com/spreadsheets/d/1uhtWqk0hYLP9kd3SxUXk_oXCRZl2A17EKXyHfAT2Q_Y/edit?usp=sharing)

* Note: There are several other parameters in *config.h*, which are unusable (or not fully tested) in the current version.

* Different protocols are implemented in different branches. 

  * Experiments without failures are done in main, hotstuff, and rcc.
  * Experiments with failures are done in main, hotstuff_recovery and rcc_recovery.

  | Branch            | Implemented Protocol                         |
  | ----------------- | -------------------------------------------- |
  | main              | PVP, PVP with recovery mechanism             |
  | hotstuff          | HotStuff and Narwhal                         |
  | hotstuff_recovery | HotStuff and Narwhal with recovery mechanism |
  | rcc               | RCC and PBFT                                 |
  | rcc_recovery      | RCC and PBFTwith recovery mechanism          |