# 01-processes: Assignment Prompts

## Changes from previous assignment's template code
- A `get_random_header()` is added to create a random header.
- A `mine_single_block()` is added. This function brute forces a block header, like we did in part 00.

### Changes from part 00alt
- Say goodbye to the merkle tree for some time. For now, we'll just deal with the header.

## Task description
Write a loop to `fork()` children. Save the PIDs of all children.

The children should run the following code first to finish setting up a signal handler:

```c
sigact.sa_handler = sig_hand;
sigact.sa_mask = set;
sigact.sa_flags = 0;
sigaction(SIGUSR1,&sigact,0);

sigdelset(&set,SIGUSR1);
sigprocmask(SIG_SETMASK, &set, NULL);
```

Then the children should run `mine_single_block()` to brute force the nonce, before exiting (`exit(0)` or `return 0`).

The parent should `wait()` one child to finish, and save its PID. Then, for every other children's PID, use `kill(pid, SIGUSR1)` to send a signal. The signal handler `sig_hand()` will receive the signal and make the child exit.

Remember to wait for the children you killed to avoid leaving zombies behind.