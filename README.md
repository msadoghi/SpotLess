# ResilientDB: A High-throughput yielding Permissioned Blockchain Fabric.

## Steps to Run and Compile <br/>

#### We strongly recommend that first try the docker version, Here are the steps to run on a real environment:

* First Step is to untar the dependencies:

        cd deps && \ls | xargs -i tar -xvf {} && cd ..
* Create **obj** folder inside **resilientdb** folder, to store object files. And **results** to store the results.

        mkdir obj

* Create a folder named **results** inside **resilientdb** to store the results.

        mkdir results

* Adjust the parameters in config.h such as number of replicas and clients
* Compile the code. On compilation, two new files are created: **runcl** and **rundb**. Each machine is going to act as a client needs to execute **runcl**. Each machine is going to act as a replica needs to execute **rundb**. 

        make clean
        make -j8

* Collect the IP address of replica and client machines in *scripts/hostnames.py*
* Generate *iplist.txt*

        python ifconfig.py

* Set up replica machines and client machines
        cd scripts
        python nodeModify.py
        cd ..

* Copy the binary file to replica machines and client machines

        python scripts/StopSystem.py
        python scripts/scp_binaries.py

* Run

        python scripts/RunSystem.py

* Get results from the replica and client machines 

        python scripts/scp_results.py


### Relevant Parameters of "config.h"
<pre>
* NODE_CNT			Total number of replicas, minimum 4, that is, f=1.  
* THREAD_CNT			Total number of threads at primary
* REM_THREAD_CNT		Total number of input threads at a replica 
* SEND_THREAD_CNT		Total number of output threads at a replica 
* CLIENT_NODE_CNT		Total number of clients (at least 1).  
* CLIENT_THREAD_CNT		Total number of threads at a client (at least 1)
* CLIENT_REM_THREAD_CNT		Total number of input threads at a client
* SEND_THREAD_CNT		Total number of output threads at a client
* MESSAGE_PER_BUFFER            Message Buffer Size
* MAX_TXN_IN_FLIGHT		Maximum value of in-flight transactions that clients send in flight
* DONE_TIMER			Amount of time to run the system.
* WARMUP_TIMER			Amount of time to warmup the system (No statistics collected).
* BATCH_THREADS			Number of threads at primary to batch client transactions.
* BATCH_SIZE			Number of transactions in a batch (at least 10)
* ENABLE_CHAIN			Set it to true if blocks need to be stored in a ledger.
* TXN_PER_CHKPT			Frequency at which garbage collection is done.
* USE_CRYPTO			To switch on and off cryptographic signing of messages.
* CRYPTO_METHOD_RSA		To use RSA based digital signatures.
* CRYPTO_METHOD_ED25519		To use ED25519 based digital signatures.
* CRYPTO_METHOD_CMAC_AES	To use CMAC + AES combination for authentication.
