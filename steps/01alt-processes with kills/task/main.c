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
#include <signal.h>

#include "sha2.h"

typedef struct{
    int version;
    char previous_block_hash[32];
    char merkle_root[32];
    int timestamp;
    int difficulty;
    int nonce;
} BitcoinHeader;


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

void sig_hand(int sig){
    printf("%d> signal %d received -- terminating gracefully\n", getpid(), sig);
    exit(0);
}


int main(){
    srand(time(NULL));
    
    int difficulty=0x1f03a30c;
    char target[32];
    construct_target(difficulty, &target);
    
    int num_processes=5;
    
    BitcoinHeader block;
    get_random_header(&block, difficulty);
    
    
    sigset_t set;
    struct sigaction sigact;
    sigfillset(&set);
    sigprocmask(SIG_SETMASK, &set, NULL);
    
    int i;
    pid_t pid_father = getpid();
    pid_t pid_child[num_processes];
    pid_t first_finisher;
    
    // TODO: fork
    /*
    // child processes should run the following code first:
    sigact.sa_handler = sig_hand;
    sigact.sa_mask = set;
    sigact.sa_flags = 0;
    sigaction(SIGUSR1,&sigact,0);
    
    sigdelset(&set,SIGUSR1);
    sigprocmask(SIG_SETMASK, &set, NULL);
    */
    // use mine_single_block(), pass in pointer for `block` and `target`, to
    // brute force the nonce
    
    
    
    return 0;
    
}



























