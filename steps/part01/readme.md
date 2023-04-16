# 01-processes: Assignment Prompts

## Task description

Write a loop to `fork()` children. Save the PIDs of all children.

The parent should wait for *one* child to finish and exit, and save its PID. Once one child has exited, send a signal to every other children to make them exit prematurely. Use the `kill` function:

```c
kill(pid, SIGUSR1);
```

to send the signal to the process with the PID you provided.

Make sure to also wait for all children you killed so that you don't leave zombie processes behind.

The children will set up the signal handler, brute force the block's nonce, and exit after they finish - the code for this part has been given in the template.