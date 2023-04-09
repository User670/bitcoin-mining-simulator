#ifndef CUSTOM_ERRORS_H
#define CUSTOM_ERRORS_H

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
*/

#define E_CUSTOM_BADCHAINLEN 6
/*
deserialize_blockchain: Read negative length.
*/
void perror_custom(int errnum, char* prefix);

#endif
