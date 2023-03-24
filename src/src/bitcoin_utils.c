#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <string.h>
#include <inttypes.h>
#include <fcntl.h>

#include <sha2.h>
#include <bitcoin_utils.h>

// requires `sha2.c`, `sha2.h`
// computes double SHA - that is, SHA twice - for a given piece of input.
// @params
//  *message: pointer to the data to be hashed
//  len: how long, in bytes, the input is
//  *digest: pointer to a 32-byte buffer to store the hash
// @return
//  void
void dsha(const unsigned char* message, unsigned int len, unsigned char* digest){
    sha256(message, len, digest);
    sha256(digest, 32, digest);
}

// converts a "difficulty" value (a 32-bit integer) to a target hash that can be
// compared against
// @params
//  d: the difficulty value
//  *target_storage: pointer to a 32-byte buffer to store the target
// @return
//  void
void construct_target(int d, const char* target_storage){
    // first two hex digits in the difficulty is the exponent
    // last 6 are the coefficient
    // target = coefficient * 2^(8*(exponent-3))
    // which kinda means place the coefficient (exponent-3) bytes from the right, when written it big-endian
    // this implementation is likely flawed but should work for this purpose
    int coefficient=d%0x1000000;
    int exponent=d>>24;
    int placement_position=32-exponent;
    memset(target_storage, 0, 32);
    memset(target_storage+placement_position, coefficient>>16, 1);
    memset(target_storage+placement_position+1, (coefficient>>8)%0x100, 1);
    memset(target_storage+placement_position+2, coefficient%0x100, 1);
}

// checks whether a block is good.
// @params
//  *block: pointer to a BitcoinHeader
//  *target: pointer to a 32-byte buffer containing the target
// @return
//  1 or 0, to be interpreted as bool, whether the header's hash is below target
int is_good_block(BitcoinHeader* block, const char* target){
    char hash_storage[32];
    dsha(block, sizeof(BitcoinHeader), &hash_storage);
    if (memcmp(hash_storage, target, 32)<0){
        return 1;
    }else{
        return 0;
    }
}

// calculate hash of two hashes combined.
// params
//  *a: pointer to a 32-byte buffer holding the first hash
//  *b: pointer to a 32-byte buffer holding the second hash
//  *digest: pointer to a 32-byte buffer to store the result
// returns
//  void
void merkle_hash(char* a, char* b, char* digest){
    char s[64];
    memcpy(&s, a, 32);
    memcpy((&s[32]), b, 32);
    dsha(&s, 64, digest);
}

// Update a block's merkle tree hashes and put the root hash in the header.
// params
//  *block: a POINTER to the block. Not header. Block.
// returns
//  void
void update_merkle_root(BitcoinBlock* block){
    // has to take a pointer
    // taking the block itself makes C duplicate the value for the
    // function, and unable to modify it outside
    calculate_merkle_root_top_down(block->merkle_tree);
    memcpy(&(block->header.merkle_root),&(block->merkle_tree->hash),32);
}

// Recursively calculate all hashes in a merkle tree.
// params
//  *node: pointer to the root node
// returns
//  void
void calculate_merkle_root_top_down(MerkleTreeHashNode* node){
    if(node->data==NULL){
        // has left and maybe right hash nodes as children
        // calculate hash of their hashes combined
        if(node->left==NULL){
            perror("calculate_merkle_root_top_down: A non-data node of a merkle tree node has to have a left child.");
            exit(1);
        }else{
            calculate_merkle_root_top_down(node->left);
        }
        if(node->right==NULL){
            // when lacking a right child, left's hash is repeated
            merkle_hash(&(node->left->hash),&(node->left->hash),&(node->hash));
        }else{
            calculate_merkle_root_top_down(node->right);
            merkle_hash(&(node->left->hash),&(node->right->hash),&(node->hash));
        }
    }else{
        // has a data node as child
        // calculate hash of that data
        dsha(&(node->data->data), node->data->length, &(node->hash));
    }
}