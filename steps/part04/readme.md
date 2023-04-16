# 04-files: Assignment Prompts

## Task description

For this part, you will work on your code from part **03-Threads**. If you think you did not do well on that part, you may ask your instructor for a fresh template to work from.

Modify your code, so that the blockchain is read from or written to files. More specifically:

- Whenever a block is mined, write the blockchain to a new file. Don't overwrite old blockchain files.
  - You can name the files in whatever naming scheme you like, as long as each file gets a unique name. Some ideas include incrementing numbers, or using the last block's hash.
  - You will also need to write down the last file's name to another file with a known file name.
- When the program starts, try to read the last blockchain file's name, then try to read the blockchain. If they exist, load the blockchain in (and skip the part where it generates a dummy genesis block). If they don't, start with the dummy genesis block as usual.

Here are some code snippets to help you get started.

```c
// write a blockchain to a file
// fd is a file descriptor, genesis_block_name is the shared memory name of the
// first block in the blockchain (should be `sd->genesis_block_name` for your
// previous parts).
write_blockchain_to_file(fd, genesis_block_name);

// read a blockchain from a file
char name[100];
// fd is a file descriptor
read_blockchain_from_file(fd, name);
// `name` now contains the shared memory name of the first block
// we can now copy it to the shared data for child processes to use
strcpy(sd->genesis_block_name, name);

// obtain the last block's hash in a blockchain, and convert it to a hex string
char hash[32];
char hex_string[65]; // 65 long is needed because of the NUL character at the end
get_last_block_hash(genesis_block_name, hash);
bytes_to_hex_string(hash, 32, hex_string);
```