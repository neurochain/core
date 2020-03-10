# Consensus

## Overall

The consensus is responsible for
* checking the validity of blocks and transactions
* choosing the MAIN branch
* computing the assemblies PIIs
* mining blocks
* cleaning up the transaction pool

## Entrypoints

The important entry points are
* *add_transaction*: check that a transaction is valid and add it to the transaction pool
* *add_block*: add a block as DETACHED or UNVERIFIED to the ledger and then call synchronously *verify_blocks*
* *add_block_async*: add a block as DETACHED or UNVERIFIED to the ledger and call *process_blocks* which will call asynchronously *verify_blocks*

## *verify_blocks*

*verify_blocks* is responsible for verifying UNVERIFIED blocks, setting them as FORK and then setting the longest branch as MAIN.

## Threads

### Compute pii thread
This thread is started at the creation of a Consensus object by the *start_compute_pii_thread* method.
Each will regularily check if there is an assembly to compute and if there is start the computation.
*compute_pii_sleep* in the consensus config determines how long the thread waits before checking again if there is an assemly to process.

### Update heights thread
This thread is started at the creation of a Consensus object by the *start_update_heights_thread* method.
This thread is responsible for writing a list of heights that should be mined by the bot later. Those heights are used by the miner thread to know which blocks it should mine. It allows the miner thread to react more quickly when it has a block to mine.
*update_heights_sleep* in the consensus config determines how long it waits between each call to *update_heights_to_write*.

### Miner thread
This thread is started at the creation of a Consensus object by the *start_miner_thread* method.
It is responsible for regularly calling *mine_block* which will use the heights to write to determine if it should mine a block.
*miner_sleep* in the consensus config determines how long the thread waits between calls to *mine_block*.

### Process blocks thread
This thread is started when *process_blocks* is called and the thread is not already running. It is responsible for calling *verify_blocks* asynchronously. *process_blocks* is called by *add_block_async*.

### Check signatures threads
Those threads are managed by a threading pool that is created if the consensus object is constructed with an argument *nb_check_signatures_threads* greater than 1.
When verifying the transactions of a blocks each worker thread will be responsible for checking that a fraction of the transactions are valid, so that it makes it faster to verify blocks on computers with multiple cores.
