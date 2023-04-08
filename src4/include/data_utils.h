#ifndef DATA_UTILS_H
#define DATA_UTILS_H
#include <bitcoin_utils.h>

void get_random_hash(void* digest);

void get_random_header(BitcoinHeader* header, int difficulty);

void get_random_continuation_header(BitcoinHeader* header, void* prev_blk_hash, int difficulty);

void get_random_transaction(MerkleTreeDataNode* node);

void obtain_last_block_hash(BitcoinBlock* genesis, void* digest);

int serialize_data_node(MerkleTreeDataNode* node, int max_size, void* buf);

int serialize_merkle_tree(MerkleTreeHashNode* node, int max_size, void* buf);

int serialize_block(BitcoinBlock* block, int max_size, void* buf);

int serialize_blockchain(BitcoinBlock* genesis, int max_size, void* buf);

int deserialize_data_node(void* serialized_buf, MerkleTreeDataNode* obj);

int deserialize_merkle_tree(void* serialized_buf, MerkleTreeHashNode* obj);

int deserialize_block(void* serialized_buf, BitcoinBlock* block);

int deserialize_blockchain(void* serialized_buf, BitcoinBlock* genesis);

#endif