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

// for multiprocessing
// mines a single header
// @params
//  *block: pointer to a block to be mined
//  *target: pointer to a expanded target
// @return
//  void
void mine_single_block(BitcoinHeader* block, const char* target){
    int i=0;
    for(; i<=2147483647; i++){
        block->nonce=i;
        if(is_good_block(block, target)){
            printf("CHILD PROCESS: nonce found %d\n", i);
            break;
        }
        
    }
}

typedef struct{
    int result_found;
    BitcoinHeader block;
    char* target;
    int no_more_jobs;
}SharedData1;

void process_miner(int id, SharedData1* sd){
    printf("CHILD %d: spawned\n",id);
    // prepare semaphore references
    sem_t* issue_job_sync_sem=sem_open("/issuejob", O_RDWR);
    sem_t* job_end_sync_sem=sem_open("/jobend", O_RDWR);
    sem_t* result_found_mutex=sem_open("/resultfound", O_RDWR);
    
    // prepare vars
    BitcoinHeader working_block;
    
    // main loop
    while(1){
        // wait for parent to issue job and release lock
        //printf("CHILD %d: I'm waiting for parent to release issuejob\n",id);
        sem_wait(issue_job_sync_sem);
        //printf("CHILD %d: I'm released from issuejob\n",id);
        // check `sd` for no-more-jobs signal
        if(sd->no_more_jobs){
            printf("CHILD %d: No more job flag raised, breaking out of main loop\n", id);
            break;
        }
        
        // make a copy of the block so that it doesn't mess with other processes
        working_block=sd->block;
        
        //printf("CHILD %d: I'm entering main loop\n",id);
        for(int i=0; i<=2147483647; i++){
            if(sd->result_found){
                // could have merged the conditional in the loop, but
                // 1, I don't like cramming conditions in the loop,
                // 2, I can print stuff here this way
                printf("CHILD %d: Someone found result; breaking at i=%d\n", id, i);
                break;
            }
            working_block.nonce=i;
            if(is_good_block(&working_block, sd->target)){
                //printf("CHILD %d: I found a nonce, waiting for mutex\n", id);
                sem_wait(result_found_mutex);
                //printf("CHILD %d: I'm in the mutex\n", id);
                // *actually* make sure no one beat me to it
                if(sd->result_found){
                    printf("CHILD %d: I found a nonce, but someone beat me to it in writing it to the shared memory.\n",id);
                }else{
                    sd->result_found=1;
                    sd->block.nonce=i;
                    printf("CHILD %d: found a valid nonce %d\n", id, i);
                }
                sem_post(result_found_mutex);
                //printf("CHILD %d: I'm out of the mutex\n", id);
                break;
            }
        }
        
        // tell parent that I'm done
        sem_post(job_end_sync_sem);
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

void debug_print_hex(void* data, int bytes){
    for(int i=0; i<bytes; i++){
        printf("%02X",*(unsigned char*)(data+i));
    }
}

void debug_print_hex_line(void* data, int bytes){
    for(int i=0; i<bytes; i++){
        printf("%02X",*(unsigned char*)(data+i));
    }
    printf("\n");
}

int main(){
    // seed the random number generator
    //srand(time(NULL));
    
    // C warm up: brute force one block
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    char hash1[32]="bix9mFWm2NQLi6UY6u4B0MdU6eLbwWLd";
    char hash2[32]="GOKapMZXhEySXqki11jjDsPYOah29naz";
    char hash3[32]="V3UTzhOMIxL9eQPP3eRPWsYuztP7rtB7";
    
    
    MerkleTreeNode* node1=malloc(sizeof(MerkleTreeNode));
    node1->is_data_node=1;
    memcpy(&(node1->hash),&hash1,32);
    node1->left=NULL;
    node1->right=NULL;
    
    MerkleTreeNode* node2=malloc(sizeof(MerkleTreeNode));
    node2->is_data_node=1;
    memcpy(&(node2->hash),&hash2,32);
    node2->left=NULL;
    node2->right=NULL;
    
    MerkleTreeNode* node3=malloc(sizeof(MerkleTreeNode));
    node3->is_data_node=1;
    memcpy(&(node3->hash),&hash3,32);
    node3->left=NULL;
    node3->right=NULL;
    
    MerkleTreeNode* node12=malloc(sizeof(MerkleTreeNode));
    node12->is_data_node=0;
    node12->left=node1;
    node12->right=node2;
    
    MerkleTreeNode* node34=malloc(sizeof(MerkleTreeNode));
    node34->is_data_node=0;
    node34->left=node3;
    node34->right=NULL;
    
    MerkleTreeNode* node1234=malloc(sizeof(MerkleTreeNode));
    node1234->is_data_node=0;
    node1234->left=node12;
    node1234->right=node34;
    
    BitcoinBlock block;
    block.merkle_tree=node1234;
    
    update_merkle_root(&block);
    
    debug_print_hex_line(&(block.header.merkle_root), 32);
    // should be:
    // 22CF135928B9B43FB0B771C552D58CA343A74B3F5C59E928A282B448742021E1
    
    block.header.version=4;
    memset(&(block.header.previous_block_hash), 0, 32);
    block.header.timestamp=time(NULL);
    block.header.difficulty=difficulty;
    block.header.nonce=0;
    
    for(int i=0; i<2147483647; i++){
        block.header.nonce=i;
        if(is_good_block(&(block.header), &target)){
            printf("Nonce found: %d\n", i);
            break;
        }
    }
    
    return 0;
    
}



























