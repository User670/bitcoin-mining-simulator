# 00-warmup with Merkle tree: Assignment Prompts

## Background info

### Merkle tree
Transactions in a Bitcoin block are organized into a Merkle tree.

Merkle trees are binary trees of hashes. The bottom layer of the tree contains hashes of each transaction, and every layer up, each node takes hashes from its children, put them together, and calculate a new hash. This repeats until a layer with only one hash. If there are an odd number of nodes on a layer, then the last node on the layer above (which only has one child) takes its child's hash *twice* and calculate a hash.

For example, a block with 3 transactions would have a tree that looks like this:

```
       c
      / \
     /   \
    /     \
   a      b
  / \    /
 1   2  3

where
1, 2, 3 are hashes of transactions,
a = hash(1 concat 2)
b = hash(3 concat 3)
c = hash(a concat b)
```

You can read more about Merkle trees in Bitcoin here: https://en.bitcoin.it/wiki/Protocol_documentation#Merkle_Trees . The Bitcoin Wiki is also a good place to know more about the inner workings of Bitcoin, if you ever get curious.

### Bitcoin mining

Bitcoin uses a "proof of work" system. The philosophy and purpose of this aside, in practice this means many Bitcoin users (called "miners") try to compete in solving a problem that takes a lot of computation power (the process is called "mining"), and whoever solves the problem first is given some bitcoins as reward.

The problem in question is to make the hash of the **block header** smaller than a certain value (the "target", derived from another value called "difficulty"). This is done by changing a value called the **nonce** in the header; every time the nonce is incremented, the hash of the header changes. Since hashes are unpredictable, this essentially becomes a guessing game where miners brute force through different nonce values one after another.

## Task description

The code template defined three structs, `BitcoinBlock` which contains a `header` and a `merkle_tree`, `MerkleTreeNode` with which to build the tree, and `BitcoinHeader` with various fields that a header contains.

First, construct a Merkle tree that has 3 transactions.

- Create `MerkleTreeNode`s, using `malloc()` to allocate memory for them.
- For bottom layer nodes, set `is_data_node` to 1, and use `memcpy()` to copy the hashes. (It is recommended to copy the random strings provided in the template, so that you can check the results.)
- For other nodes, set `is_data_node` to 0, and assign left and right pointers to point to its children nodes. If a node only has 1 child, set the child to `left`, and set `right` to `NULL`.

After you construct the tree, find the variable declaration `BitcoinBlock block`, and set `block.merkle_tree` to the pointer to the root node of your tree. The `update_merkle_root()` function will calculate all the hashes.

Next, try to write a loop to mine this block. Increment `block.header.nonce` each time, and use `is_good_block()` to check if the block has a hash that is below the target. When you found a valid nonce, print it out. The `target` was given at the beginning of the `main()` function.

Finally, don't forget to `free()` the memory you allocated.