# 01-processes: Assignment Prompts

## Changes from previous assignment's template code
- A `get_random_header()` is added to create a random header.
- A `mine_single_block()` is added. This function brute forces a block header, like we did in part 00.

## Task description
In `main()`, complete the `for` loop to spawn child processes; each process will use `mine_single_block()` to brute force a different block header.

After that, the parent process should `wait()` for all processes to exit before it can exit (`return 0;`).