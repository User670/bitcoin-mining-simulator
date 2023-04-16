Here documents what I did from cleaned-up source code to templates.

## part01alt

- Removed the for loop to create forks.

```c
    // TODO: write a loop to fork.
    // Make sure to save the PID of each child.
```

- Removed the if's condition (replaced with `/* TODO: how can you tell who is the parent? */`).
- Removed the parent part of the if - the child part, aka the else, was left intact.

```c
        // TODO: Parent: wait for one child, kill the rest, and wait for them.
```
