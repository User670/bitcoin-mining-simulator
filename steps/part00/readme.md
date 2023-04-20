# 00-C Warmup: Assignment Prompt

## Task Description

Try to complete the following steps.

### 1. Compile and run the code.
Compile and run the template code as it is.

You should be able to compile the code with `make build` and run it with `./bin/main.bin`.

If it runs successfully, it should print the header for the Genesis block of Bitcoin. It should look like this:

```
====Debug print header start====
version:       1
prev blk hash: 0000000000000000000000000000000000000000000000000000000000000000
merkle root:   3BA3EDFD7A7B12B27AC72C3E67768F617FC81BC3888A51323A9FB8AA4B1E5E4A
timestamp:     1231006505 (2009-01-04 02:15:05 (CST))
difficulty:    1D00FFFF
nonce:         2083236893
====Debug print header end====
```

### 2. Edit the code to complete the following
- Create another BitcoinBlock, by declaring a pointer and allocating memory for it using `malloc()`.\
  While it is still possible to simply create a BitcoinBlock variable, it is also useful to learn to work with pointers and memory management.
- Use `initialize_block` to initialize the block, and use `randomize_block_transactions` to give the block a random set of transactions.\
  The former is found in `bitcoin_utils.c`, the latter is found in `data_utils.c`. You can find the source code and documentation about them in their respective files.
- Write a loop to "mine the block". This is done by brute force: give the block a `nonce` value, check if the hash satisfies a condition, and if it doesn't, try again with a different value. Print a valid nonce that it finds.\
  Use `is_good_block` from `bitcoin_utils.c` to perform the check.
- Finally, remember to free the memory you allocated with `malloc` by calling `free`.

### 3. Re-compile and re-run your code.
See if it runs properly. Check for any errors and bugs.
