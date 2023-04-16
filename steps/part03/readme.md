# 03-Threads: Assignment Prompts

## Task description

For this part, you will work on your code from part **02-semaphores and shared mem**. If you think you did not do well on that part, you may ask your instructor for a fresh template to work from.

Modify your children's code. Each child should spawn a number of threads, and the brute force job would be done by the threads.

Synchronize the processes and threads, so that once one thread finds a valid nonce, all other threads across the processes should stop and wait for a new task. The code responsible for writing the block to a shared memory may stay in the child process's main thread, or it may be done by the threads it spawns - your choice.

You may kill and re-spawn threads between tasks if you wish.

You might want to define a new macro like `#define NUM_THREADS 5` near the top of your `main.c` file.