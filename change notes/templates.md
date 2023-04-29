Here documents what I did from cleaned-up source code to templates.

## part01alt

- Removed the for loop to create forks.
- Removed the if's condition.
- Removed the parent part of the if - the child part, aka the else, was left intact.


## part02

- Kept only `new_block`, `genesis_block_name` and `target` in SharedData; yanked the rest.
- `process_miner`: removed semaphore part; removed most of the main loop logic other than the grab-bruteforce-write bits.
- `main`: Removed the bit with the shared memory (but still left the `sd->target=target`), semaphores, the middle of the main loop, the part where it signals no more jobs, and cleaning up the semaphores and shms they made.