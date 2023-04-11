#ifndef CUSTOM_ERRORS_H
#define CUSTOM_ERRORS_H
// This is a custom implementation to imitate the purpose of errno.
// There is no errno variable - functions using this should either return
// an error code or take an int* parameter to store an error code.

#define E_CUSTOM_NOMEM 1
/*
_serialize_value: no enough space in the buffer, as indicated by the max_size
parameter.
It is possible to raise this error erroneously when there is still space in the
buffer, or write past the end of the buffer without raising this error, when
max_size does not match up with the actual remaining size of the buffer.
*/

#define E_CUSTOM_BADTREE 2
/*
serialize_merkle_tree: A merkle tree is malformatted.
*/

#define E_CUSTOM_BADDATALEN 3
/*
deserialize_data_node: Read length of zero or negative.
*/

#define E_CUSTOM_BADTREELEN 4
/*
deserialize_merkle_tree: Read length of zero or negative.
*/

#define E_CUSTOM_EMPTYCHAIN 5
/*
deserialize_blockchain: Read length of zero, indicating an empty blockchain.
Do note that the serialization function doesn't handle empty chains. If you
want to manually indicate an empty chain, write 4 bytes of zero (int value 0)
so that the length of the blockchain reads zero.
*/

#define E_CUSTOM_BADCHAINLEN 6
/*
deserialize_blockchain: Read negative length.
*/
void perror_custom(int errnum, char* prefix);

#endif
