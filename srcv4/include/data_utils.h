#ifndef DATA_UTILS_H
#define DATA_UTILS_H
#include <bitcoin_utils.h>

void get_random_hash(void* digest);

void get_random_header(BitcoinHeader* header, int difficulty);

void get_random_continuation_header(BitcoinHeader* header, void* prev_blk_hash, int difficulty);

/* void get_random_transaction(MerkleTreeDataNode* node);

void randomize_block(BitcoinBlock* block);

void obtain_last_block_hash(BitcoinBlock* genesis, void* digest);

int obtain_last_nonce(BitcoinBlock* genesis);

int serialize_data_node(MerkleTreeDataNode* node, int max_size, void* buf, int* err);

int serialize_merkle_tree(MerkleTreeHashNode* node, int max_size, void* buf, int* err);

int serialize_block(BitcoinBlock* block, int max_size, void* buf, int* err);

int serialize_blockchain(BitcoinBlock* genesis, int max_size, void* buf, int* err);

int deserialize_data_node(MerkleTreeDataNode* obj, void* serialized_buf, int* err);

int deserialize_merkle_tree(MerkleTreeHashNode* obj, void* serialized_buf, int* err);

int deserialize_block(BitcoinBlock* block, void* serialized_buf, int* err);

int deserialize_blockchain(BitcoinBlock* genesis, void* serialized_buf, int* err);

void get_dummy_genesis_block(BitcoinBlock* block); */

void bytes_to_hex_string(void* bytes, int size, void* storage);

/* int write_blockchain_to_file(int fd, int max_size, BitcoinBlock* block);

int read_blockchain_from_file(int fd, BitcoinBlock* block); */

void randomize_transaction(MerkleTreeNode* node);

void randomize_block_transactions(BitcoinBlock* block);

void get_dummy_genesis_block(BitcoinBlock* block);

int is_valid_block_shm_name(char* name);

void construct_shm_name(char* hash, char* buf);

int write_blockchain_to_file(int fd, char* name);

int read_blockchain_from_file(int fd, char* genesis_name_storage);

#endif