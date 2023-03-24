# 02-semaphores and shared mem: Assignment Prompts

## Changes from previous assignment's template code
- A struct `SharedData` is added.
- A `process_miner()` is added. It is incomplete, and you will need to complete its logic.

## Task description
You will modify `SharedData`, `process_miner()`, and `main()` for this task.

For the parent process, it should:
- spawn child processes,
- inform child processes of new block headers to mine,
- receive results from child processes.

For the child processes, they should:
- wait until parent to issue a new block header, and start mining,
- upon finding a valid nonce, inform other processes to stop working, and also inform the parent process the result.

The processes should loop to process multiple block headers one after another, without killing and re-spawning child processes.

Use shared memory and semaphores to help synchronize the processes.