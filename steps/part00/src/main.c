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
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>
#include <custom_errors.h>

int main(){
    // seed the random number generator
    srand(time(NULL));
    
    int difficulty=DIFFICULTY_1M;
    char target[32];
    construct_target(difficulty, &target);
    
    BitcoinBlock genesis_block;
    initialize_block(&genesis_block, difficulty);
    get_dummy_genesis_block(&genesis_block);
    debug_print_header(genesis_block.header);
    
    // TODO: declare a BitcoinBlock pointer and allocate memory.
    // BitcoinBlock* my_block = ...
    
    // TODO: initialize the block and add random transactions
    // initialize_block(my_block, difficulty);
    // randomize_block_transactions(my_block);
    
    // TODO: write a for loop
    // if( is_good_block(my_block, target) ) ...
    
    return 0;
    
}



























