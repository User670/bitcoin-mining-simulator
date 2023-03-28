I might work here.

This folder might contain live code, or some code that is a few versions older, depending on where I happened to be working last.

**src4**: for part 4 and onwards. Mainly changes bitcoin structure related code (changes from an explicit tree with nodes and pointers to an array representation). Why? It's easier to deal with (no more navigating the tree left and right, up and down)!

## File format definitions

Transaction data begin with an int (number of transactions), then that many of the following: an int (how long the transaction is), followed by the transaction itself.

Block data is the header followed by transaction data.

Blockchain data is multiple block data next to each other. ~~Not sure if there is need for "number of blocks" metadata at the beginning.~~ An int at the beginning for block count.

Header data is the same as how the C program internally stores it, which should be similar to the real Bitcoin header format (int version, char[32] previous block hash, char[32] merkle root, int timestamp, int difficulty bits, int nonce. 80 bytes total).

The program does not enforce big/little endian of ints, nor how the header is actually represented. It might be incompatible if the program is ran on two different platforms / compiled by two different compilers, and one tries to read file created by the other.