# Short term goals

## Phase 1/4 Multi processing with synchronization

### Part 0 - C language warm up
- [X] Copy over the SHA and header format related code
- [X] Have a functional target check, and a functional brute force loop
- ~~Current issue: is this enough of a warm up for this task?~~ No, need to be beefed up
- [ ] Make a block chain structure. This will affect all parts downstream

### Part 1 - Basic multi processing
- [X] Parent: create multiple BitCoin headers for the threads to crack
- [X] Parent: fork
- [X] Child: brute force and print result
- [ ] Re-do: Child returns exit value; parent kills other children

### Part 2 - Synchronization
- [ ] Challenge: What way should we use to inform other processes when a process finds a result?
  - [X] Found a way to sync with just a boolean in shared mem. Doing it with, uh, some way that has semaphore in it.

## Phase 2/4 Threads

### Part 3 - Threads
- [ ] Child: spawn threads
- [ ] Threads: mine blocks
- [ ] Still need to be synced and mutex'd