# 03-Threads

For this assignment, start with your code in part 02. You may also ask for a fresh template for this assignment if you did not complete your previous assignment or if you are not confident with your previous work.

## Task description

Modify your code, so that each child process creates a number of threads that do the mining.

As soon as one thread finds a valid nonce, all threads in all child processes should stop mining and wait for the next task.

You may have the threads exit and be re-created between tasks.

The many threads from one child process are cooperative (as opposed to the many child processes, who are competitive). Try to make the threads cooperate more efficiently if you can.