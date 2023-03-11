# 00-warmup: Assignment Prompts

## Task description
In `main()`, write a loop that tries to compute a valid nonce for a Bitcoin block header by brute force.

Use `is_good_block()` to check whether your nonce is good. You need to pass in two parameters, a *pointer* to the header, and a *pointer* to the "target" that the hash needs to compare to.