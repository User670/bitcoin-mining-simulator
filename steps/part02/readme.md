# 02-semaphores and shared mem: Assignment Prompts

Now that we have learned about shared memories, we can put the last bit about our mockup Bitcoin block into action.

We are going to store each block in the blockchain in its shared memory. Each of such shared memory would have a unique name, namely `/btcblock-` followed by the block's header hash, represented by 64 hexadecimal digits. For example:

```
/btcblock-0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef
```

The `next_block` and `previous_block` fields of each block should contain the next or previous block's shared memory's name.

However, don't get overwhelmed by this - we've done enough heavy lifting for you.

## Task description

Your main focus is to write synchronization code to coordinate the parent distributing blocks to mine, and the children mining them, using semaphores and shared memories.

Here's how it should work:

- **Parent** prepares a block, and signals children to start working.
- **Children** start to brute force. Once one children finds a valid nonce, it should signal other children to stop working, as well as signal the parent.
- **Children**, upon receiving signal from a sibling that has found a nonce, should stop mining, and also signal the parent.
- **Parent** confirms the result.

This should be able to loop multiple times without having to kill and re-spawn child processes.