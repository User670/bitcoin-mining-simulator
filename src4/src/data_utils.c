#include <bitcoin_utils.h>
#include <sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>

// gives a random 32-byte hash.
// For simplicity, it just hashes a random integer.
// @params
//  *digest: pointer to a 32-byte buffer to store the hash
// @return
//  void
void get_random_hash(unsigned char* digest){
    int a=rand();
    dsha(&a, sizeof(int), digest);
}

// gives a random bitcoin header.
// @params
//  *block: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_header(BitcoinHeader* block, int difficulty){
    block->version=4;
    get_random_hash(&(block->previous_block_hash));
    get_random_hash(&(block->merkle_root));
    block->timestamp=time(NULL);
    block->difficulty=difficulty;
    block->nonce=0;
}

// gives a random bitcoin header.
// @params
//  *block: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_continuation_header(BitcoinHeader* block, char* prev_blk_hash, int difficulty){
    block->version=4;
    memcpy(&(block->previous_block_hash), prev_blk_hash, 32);
    get_random_hash(&(block->merkle_root));
    block->timestamp=time(NULL);
    block->difficulty=difficulty;
    block->nonce=0;
}

// Initializes a block with random transactions.
// Will overwrite what was already in the block.
// Will not free memory for previous data in the block. If there is any
// data, free the memory before calling to prevent memory leak.
// params
//  *block: pointer to a block
// return
//  void
void get_random_block_transactions(BitcoinBlockv2* block){
    // random integer between 5 and 29 - # of transactions to generate
    int num_of_transactions=rand()%25+5;
    block->data_count=num_of_transactions;
    block->data=malloc(num_of_transactions*sizeof(MerkleTreeDataNode));
    for(int i=0; i<num_of_transactions; i++){
        get_random_transaction((block->data)+i);
    }
}

// Initializes a MerkleTreeDataNode with a random transaction.
// Will overwrite what was already in the node.
// Will not free memory for previous data in the node. See above.
// params:
//  *node: pointer to a node
// return
//  void
void get_random_transaction(MerkleTreeDataNode* node){
    // random integer between 128 and 511 - length of transaction
    int length=rand()%384+128;
    node->length=length;
    node->data=malloc(length);
    for(int i=0; i<length; i++){
        *((node->data)+i)=(char)(rand()%256);
    }
}


// Dump transactions in a block to a file descriptor.
// params
//  fd: a file descriptor
//  block: a block.
// return
//  void
void dump_transactions(int fd, BitcoinBlockv2 block){
    /*
    // write magic number "OSBT" (OS Bitcoin Transactions)
    write(fd, "OSBT", 4);
    */
    // write # of transactions
    write(fd, &(block.data_count), sizeof(int));
    
    // for each transaction in block...
    for(int i=0; i<block.data_count; i++){
        // write length
        write(fd, &((block.data+i)->length), sizeof(int));
        // write data
        write(fd, (block.data+i)->data, (block.data+i)->length);
    }
    close(fd);
}


// Read a file descriptor for transactions, and load it into a block.
// Does not update merkle root - do it yourself after the read.
// params
//  fd: a file descriptor
//  *block: pointer to a block
// return
//  void
void load_transactions(int fd, BitcoinBlockv2* block){
    /*
    // read and compare magic number
    char magic_number[4];
    read(fd, &magic_number, 4);
    if(memcmp(&magic_number, "OSBT", 4)){
        printf("load_transactions: Wrong magic number\n");
        close(fd);
        exit(1);
    }
    */
    // read data count
    read(fd, &(block->data_count), sizeof(int));
    block->data=malloc(block->data_count*sizeof(MerkleTreeDataNode));
    
    int l;
    // read data
    for(int i=0; i<block->data_count; i++){
        read(fd, &l, sizeof(int));
        ((block->data)+i)->length=l;
        ((block->data)+i)->data=malloc(l);
        read(fd, ((block->data)+i)->data, l);
    }
    
    close(fd);
}

// Dump a block v2, including its transactions, to a file.
// params
//  fd: a file descriptor
//  block: the block
// return
//  void
void dump_block(int fd, BitcoinBlockv2 block){
    write(fd, &block.header, sizeof(BitcoinHeader));
    dump_transactions(fd, block);
}

// Read a blockchain file, and calculate the last block header's hash.
// Relies on the block count metadata to be correct.
// Will get slow if chain gets very long, as it seeks through the blocks
// one by one.
// params
//  fd: a file descriptor
//  *target: place to store the hash
// return
//  void
void obtain_last_block_hash(int fd, char* target){
    int block_count;
    int transaction_count;
    int transaction_length;
    
    // read # of blocks
    read(fd, &block_count, sizeof(int));
    
    for(int i=0; i<block_count-1; i++){
        // skip header
        lseek(fd, sizeof(BitcoinHeader), SEEK_CUR);
        // read transaction count
        read(fd, &transaction_count, sizeof(int));
        for(int j=0; j<transaction_count; j++){
            // read transaction length
            read(fd, &transaction_length, sizeof(int));
            // skip the transaction
            lseek(fd, transaction_length, SEEK_CUR)
        }
    }
    
    BitcoinHeader block;
    // read the remaining header
    read(fd, &block, sizeof(BitcoinHeader));
    // hash the header
    dsha(&block, sizeof(BitcoinHeader), target);
}

// Get a blockchain file's block count by reading the metadata.
// param
//  fd: a file descriptor
// return
//  number of blocks
int obtain_block_count(int fd){
    int a;
    read(fd, &a, sizeof(int));
    return a;
}


