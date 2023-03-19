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

#include "sha2.h"

typedef struct{
    int version;
    char previous_block_hash[32];
    char merkle_root[32];
    int timestamp;
    int difficulty;
    int nonce;
} BitcoinHeader;

typedef struct MerkleTreeNode{
    char hash[32];
    int is_data_node; // use as boolean
    struct MerkleTreeNode* left;
    struct MerkleTreeNode* right;
} MerkleTreeNode;

typedef struct{
    BitcoinHeader header;
    MerkleTreeNode* merkle_tree;
}BitcoinBlock;


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
    block->version=2;
    get_random_hash(&(block->previous_block_hash));
    get_random_hash(&(block->merkle_root));
    block->timestamp=time(NULL);
    block->difficulty=difficulty;
    block->nonce=0;
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
void calculate_merkle_root_top_down(MerkleTreeNode* node){
    if(node->is_data_node){
        return;
    }
    if(node->left==NULL){
        perror("A non-data node of a merkle tree node has to have a left child.");
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
}

// Print a continuous part of memory in hexadecimal.
// params
//  *data: pointer to the start of the memory to print
//  bytes: how many bytes to print
// return
//  void
void debug_print_hex(void* data, int bytes){
    for(int i=0; i<bytes; i++){
        printf("%02X",*((unsigned char*)data+i));
    }
}

// Same as debug_print_hex, but also prints a \n at the end.
// params
//  *data: pointer to the start of the memory to print
//  bytes: how many bytes to print
// return
//  void
void debug_print_hex_line(void* data, int bytes){
    debug_print_hex(data, bytes);
    printf("\n");
}

int main(){
    
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    char hash1[32]="bix9mFWm2NQLi6UY6u4B0MdU6eLbwWLd";
    char hash2[32]="GOKapMZXhEySXqki11jjDsPYOah29naz";
    char hash3[32]="V3UTzhOMIxL9eQPP3eRPWsYuztP7rtB7";
    
    // TODO: construct your merkle tree.
    /* hint: here's a graphical representation of the tree,
       you can also find this in the task prompt:
           c
          / \
         /   \
        /     \
       a      b
      / \    /
     1   2  3
    */
    
    
    BitcoinBlock block;
    block.merkle_tree= /* pointer to your root node */;
    
    update_merkle_root(&block);
    
    debug_print_hex_line(&(block.header.merkle_root), 32);
    // If you used the random strings provided in the template,
    // the hash that got printed out should be:
    // 22CF135928B9B43FB0B771C552D58CA343A74B3F5C59E928A282B448742021E1
    
    // fill up rest of the fields in the block header
    block.header.version=4;
    // pretend this is the genesis block, whose prev block hash is all 0
    memset(&(block.header.previous_block_hash), 0, 32);
    block.header.timestamp=time(NULL);
    block.header.difficulty=difficulty;
    block.header.nonce=0;
    
    // TODO: write a loop to mine this block (brute force the nonce)
    
    // TODO: free your allocated memory
    
    return 0;
    
}



























