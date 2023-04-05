#ifndef DATA_UTILS_H
#define DATA_UTILS_H
#include <bitcoin_utils.h>

void get_random_hash(void* digest);

void get_random_header(BitcoinHeader* block, int difficulty);

void get_random_continuation_header(BitcoinHeader* block, void* prev_blk_hash, int difficulty);

void get_random_transaction(MerkleTreeDataNode* node);

void get_random_block_transactions(BitcoinBlockv2* block);

void dump_transactions(int fd, BitcoinBlockv2 block);

void load_transactions(int fd, BitcoinBlockv2* block);

void dump_block(int fd, BitcoinBlockv2 block);

void obtain_last_block_hash(int fd, void* digest);

int obtain_block_count(int fd);

void obtain_last_block_hash_v3(BitcoinBlockv3* genesis, void* digest);

#endif