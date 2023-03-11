# 02-semaphore and shared mem
A simple program that has multiple processes coordinated by semaphores and shared memory.

## Slightly detailed explanation
- Parent spawns multiple processes.
- Each child, after doing some prep work, sleeps by waiting on a blocked semaphore.
- Parent issues a task by writing into a shared memory, wakes up children by unblocking the semaphore, then sleeps by waiting on another blocked semaphore.
- The children start brute forcing the task, while also checking a flag in the shared memory each loop to see if another process has completed the task first.
  - When one child completes the task, it blocks another semaphore (used like a mutex), writes the result to shared memory, raises the flag, and unblocks the mutex semaphore.
- Upon seeing a task completed flag, children break out of the brute force loop, unblock the semaphore that the parent is sleeping on, and sleeps like before (loops).
- Parent may now process the results written by a child, then either loop back and issue another task, or raise a "no more jobs" flag to signal the children to exit after being waken up again.

## intended functionalities
- [X] spawn processes
- [X] parent: issue tasks, and wake up children
- [X] children: perform tasks, and stop once one process has result

## Task prep progress
- [ ] code clean up
- [ ] dig holes
- [ ] write prompt text