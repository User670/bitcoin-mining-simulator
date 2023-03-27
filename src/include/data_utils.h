#pragma once
#include <bitcoin_utils.h>

void get_random_hash(unsigned char* digest);

void get_random_header(BitcoinHeader* block, int difficulty);

void get_random_continuation_header(BitcoinHeader* block, char* prev_blk_hash, int difficulty);