# 00alt-warmup with Merkle tree
A C language warm up task that has students contruct a binary tree, and also write a loop to mine a block.

## Potential problems
- The merkle tree only holds hashes, it doesn't hold transaction data itself.
  - (to make the tree also hold data, it's like, either the node struct will have another field that non-data nodes that won't use, or there will be two different types of node structs that both can potentially be a child to a node.)
- The data structure contains the entire tree with every intermediate hash in between, but a real Bitcoin block only stores the raw transactions and the header (which has the hash of the tree's ROOT, and nothing in between).
- The tree won't be used for a long time (until memory management / files). Processes and threads only deal with the headers.

## intended functionalities
- [X] Bitcoin-related functionalities
- [X] A brute force loop in `main()`

Compared to the original part 00,
- [X] Data structures and functions for merkle tree

## Task prep progress
- [ ] code clean up
- [ ] dig holes
- [ ] write prompt text