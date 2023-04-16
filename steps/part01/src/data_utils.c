#include <bitcoin_utils.h>
#include <debug_utils.h>
#include <custom_errors.h>
#include <sha2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <regex.h>
#include <sys/mman.h>



// gives a random 32-byte hash.
// For simplicity, it just hashes a random integer.
// @params
//  *digest: pointer to a 32-byte buffer to store the hash
// @return
//  void
void get_random_hash(void* digest){
    int a=rand();
    dsha((void*)&a, sizeof(int), digest);
}

// gives a random bitcoin header.
// @params
//  *header: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_header(BitcoinHeader* header, int difficulty){
    header->version=4;
    get_random_hash(&(header->previous_block_hash));
    get_random_hash(&(header->merkle_root));
    header->timestamp=time(NULL);
    header->difficulty=difficulty;
    header->nonce=0;
}

// gives a random bitcoin header.
// @params
//  *header: pointer to a header
//  difficulty: the difficulty to be written to the header
// @return
//  void
void get_random_continuation_header(BitcoinHeader* header, void* prev_blk_hash, int difficulty){
    header->version=4;
    memcpy(&(header->previous_block_hash), prev_blk_hash, 32);
    get_random_hash(&(header->merkle_root));
    header->timestamp=time(NULL);
    header->difficulty=difficulty;
    header->nonce=0;
}


// Convert a byte array to hex string.
// params
//  *bytes: the bytes to convert.
//  size: how many bytes to convert.
//  *storage: place to store the string.
// return
//  void
void bytes_to_hex_string(void* bytes, int size, void* storage){
    for(int i=0; i<size; i++){
        sprintf((char*)storage+i*2, "%02x", *(unsigned char*)(bytes+i));
    }
    // add a \0
    *(char*)(storage+size*2)=0;
}

// Randomizes a transaction.
// params
//  *node: the node to randomize.
// return
//  void
void randomize_transaction(MerkleTreeNode* node){
    // random integer between 128 and 511 - length of transaction
    int length=rand()%384+128;
    node->length=length;
    for(int i=0; i<length; i++){
        (node->data)[i]=(char)(rand()%256);
    }
}

// Randomizes a block's transactions, and update its merkle root at the same
// time.
// params
//  *block: the block to randomize.
// return
//  void
void randomize_block_transactions(BitcoinBlock* block){
    // random integer between 5 and 29 - # of transactions
    int length=rand()%25+5;
    block->tree_length=length;
    for(int i=0; i<length; i++){
        randomize_transaction(&(block->merkle_tree[i]));
    }
    update_merkle_root(block);
}

// Get a dummy genesis block, with the same data as the real Bitcoin genesis block.
// The reason why this exists is to simplify things - initializing the chain with
// a block means code don't need to deal with empty chains.
// (Yes, the dummy block's header hash has zeros on the end, we're not gonna care
// about endianness in this simulator)
// params
//  *block: an initialized block with a merkle hash node already allocated.
// return
//  void
void get_dummy_genesis_block(BitcoinBlock* block){
    block->header.version=1;
    memset(block->header.previous_block_hash, 0, 32);
    // merkle root will be updated later
    block->header.timestamp=1231006505;
    block->header.difficulty=0x1d00ffff;
    block->header.nonce=2083236893;
    
    char _data[]={
            0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff,
            0xff, 0x4d, 0x04, 0xff, 0xff, 0x00, 0x1d, 0x01,
            0x04, 0x45, 0x54, 0x68, 0x65, 0x20, 0x54, 0x69,
            0x6d, 0x65, 0x73, 0x20, 0x30, 0x33, 0x2f, 0x4a,
            0x61, 0x6e, 0x2f, 0x32, 0x30, 0x30, 0x39, 0x20,
            0x43, 0x68, 0x61, 0x6e, 0x63, 0x65, 0x6c, 0x6c,
            0x6f, 0x72, 0x20, 0x6f, 0x6e, 0x20, 0x62, 0x72,
            0x69, 0x6e, 0x6b, 0x20, 0x6f, 0x66, 0x20, 0x73,
            0x65, 0x63, 0x6f, 0x6e, 0x64, 0x20, 0x62, 0x61,
            0x69, 0x6c, 0x6f, 0x75, 0x74, 0x20, 0x66, 0x6f,
            0x72, 0x20, 0x62, 0x61, 0x6e, 0x6b, 0x73, 0xff,
            0xff, 0xff, 0xff, 0x01, 0x00, 0xf2, 0x05, 0x2a,
            0x01, 0x00, 0x00, 0x00, 0x43, 0x41, 0x04, 0x67,
            0x8a, 0xfd, 0xb0, 0xfe, 0x55, 0x48, 0x27, 0x19,
            0x67, 0xf1, 0xa6, 0x71, 0x30, 0xb7, 0x10, 0x5c,
            0xd6, 0xa8, 0x28, 0xe0, 0x39, 0x09, 0xa6, 0x79,
            0x62, 0xe0, 0xea, 0x1f, 0x61, 0xde, 0xb6, 0x49,
            0xf6, 0xbc, 0x3f, 0x4c, 0xef, 0x38, 0xc4, 0xf3,
            0x55, 0x04, 0xe5, 0x1e, 0xc1, 0x12, 0xde, 0x5c,
            0x38, 0x4d, 0xf7, 0xba, 0x0b, 0x8d, 0x57, 0x8a,
            0x4c, 0x70, 0x2b, 0x6b, 0xf1, 0x1d, 0x5f, 0xac,
            0x00, 0x00, 0x00, 0x00
        };
    
    block->tree_length=1;
    block->merkle_tree[0].length=204;
    memcpy(block->merkle_tree[0].data, _data, 204);
    update_merkle_root(block);
}

// Uses a regular expression to test whether a string is a valid name for
// a block shared memory.
// A valid name begins with "/btcblock-", followed by 64 hex digits, lowercase.
// params
//  *name: the string to test
// return
//  0 if not valid, 1 if valid.
int is_valid_block_shm_name(char* name){
    regex_t re;
    regcomp(&re, "^/btcblock-[0-9a-f]{64}$", REG_NOSUB|REG_EXTENDED);
    return !regexec(&re, name, 0, NULL, 0);
}

// Given a hash, construct the corresponding /btcblock- shm name.
// params
//  *hash: the hash.
//  *buf: a pointer to store the name to. Needs at least 65 bytes of space.
// return
//  void
void construct_shm_name(char* hash, char* buf){
    char h[65];
    bytes_to_hex_string(hash, 32, h);
    sprintf(buf, "/btcblock-%s", h);
}

// Write a blockchain to file.
// The file consists of a number (int32, 4 bytes) of how many blocks in the chain,
// followed by this many BitcoinBlock objects, directly written in their in-
// memory representation.
// params
//  fd: a file descriptor.
//  *name: name of the genesis block's shared memory.
// return
//  0 if success, a negative number if an error occurs.
int write_blockchain_to_file(int fd, char* name){
    int num_blocks=get_blockchain_length(name);
    if(num_blocks<0){
        // it's actually an error code here
        return num_blocks;
    }
    // write # of blocks
    write(fd, &num_blocks, sizeof(int));
    char name1[100];
    char name2[100];
    BitcoinBlock block;
    strcpy(name1, name);
    int rv;
    for(int i=0;i<num_blocks; i++){
        rv=get_block_info(name1, name2, NULL, &block);
        if(rv<0)return rv;
        write(fd, &block, sizeof(BitcoinBlock));
        strncpy(name1, name2, 100);
    }
    return 0;
}

// Read a blockchain from a file.
// params:
//  fd: a file descriptor
//  *genesis_name_storage: a pointer to store the genesis block's name.
// return
//  0 if success, a negative number if an error occurs.
int read_blockchain_from_file(int fd, char* genesis_name_storage){
    int num_blocks;
    read(fd, &num_blocks, sizeof(int));
    if(num_blocks<=0){
        return E_CUSTOM_GENERIC;
    }
    BitcoinBlock block;
    char hash[32];
    char genesis_name[100];
    char name[100];
    int rv;
    for(int i=0; i<num_blocks; i++){
        read(fd, &block, sizeof(BitcoinBlock));
        if(i==0){
            // genesis
            dsha(&(block.header), sizeof(BitcoinHeader), hash);
            construct_shm_name(hash, genesis_name);
            write_block_in_shm(genesis_name, &block);
            strcpy(genesis_name_storage, genesis_name);
        }else{
            // attach to chain
            rv=attach_block(genesis_name, &block, name);
            if(rv<0)return rv;
            write_block_in_shm(name, &block);
        }
    }
    return 0;
}