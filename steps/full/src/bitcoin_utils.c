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
#include <data_utils.h>
#include <custom_errors.h>

// requires `sha2.c`, `sha2.h`
// computes double SHA - that is, SHA twice - for a given piece of input.
// params
//  *message: pointer to the data to be hashed
//  len: how long, in bytes, the input is
//  *digest: pointer to a 32-byte buffer to store the hash
// return
//  void
void dsha(void* message, unsigned int len, void* digest){
    sha256(message, len, digest);
    sha256(digest, 32, digest);
}

// converts a "difficulty" value (a 32-bit integer) to a target hash that can be
// compared against
// params
//  d: the difficulty value
//  *target_storage: pointer to a 32-byte buffer to store the target
// return
//  void
void construct_target(int d, void* target_storage){
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
int is_good_block(BitcoinHeader* header, const char* target){
    char hash_storage[32];
    dsha(header, sizeof(BitcoinHeader), hash_storage);
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
void merkle_hash(void* a, void* b, void* digest){
    char s[64];
    memcpy(&s, a, 32);
    memcpy((&s[32]), b, 32);
    dsha(s, 64, digest);
}

// Initializes a block with some basic info in the header and an empty
// merkle tree.
// params
//  *block: the block to initialize.
//  difficulty: the difficulty bits.
// return
//  void
void initialize_block(BitcoinBlock* block, int difficulty){
    // There aren't pointers that might or might not be null this time...
    // but length still have to be wiped at the very least
    memset(block, 0, sizeof(BitcoinBlock));
    // should be clean now lol
    block->header.version=4;
    block->header.difficulty=difficulty;
    block->header.timestamp=time(NULL);
}

// Set a block's transaction data node at a given index.
// params
//  *block: the block to update
//  index: the index to update
//  length: length of the data in bytes
//  *data: the transaction data
// return
//  void
void set_data_node(BitcoinBlock* block, int index, int length, char* data){
    block->merkle_tree[index].length=length;
    memcpy(
        block->merkle_tree[index].data,
        data,
        length
    );
}

// Add a data node to a block, and increment its tree length counter.
// params
//  *block: the block to add to.
//  length: length of the data, in bytes.
//  *data: the transaction data.
// return
//  void
void add_data_node(BitcoinBlock* block, int length, char* data){
    set_data_node(block, block->tree_length, length, data);
    block->tree_length++;
}

// Computes the merkle root, and copy it to the header.
// params
//  *block: the block to update.
// return
//  void
void update_merkle_root(BitcoinBlock* block){
    char d[30][32];
    int this_layer_length=block->tree_length;
    int length_is_odd=this_layer_length%2;
    int next_layer_length=this_layer_length/2+length_is_odd;
    for(int i=0; i<this_layer_length; i++){
        dsha(block->merkle_tree[i].data, block->merkle_tree[i].length, d[i]);
    }
    
    while(this_layer_length>1){
        for(int i=0; i<next_layer_length; i++){
            if(i==next_layer_length-1 && length_is_odd){
                merkle_hash(d[2*i],d[2*i],d[i]);
            }else{
                merkle_hash(d[2*i],d[2*i+1],d[i]);
            }
        }
        this_layer_length=next_layer_length;
        length_is_odd=this_layer_length%2;
        next_layer_length=this_layer_length/2+length_is_odd;
    }
    
    memcpy(block->header.merkle_root, d[0], 32);
}

// Get a block's info based on its name.
// It's slightly hard to use, so it's recommended to use a wrapper for this
// function.
// params
//  *name: name of the shared memory for this block.
//  *next_block_name_storage: if not NULL, copy the next block's name here.
//  *block_hash_storage: if not NULL, copy this block's header hash here.
//  *block_storage: if not NULL, copy the entire block here. Note that this copy
//        is not mapped from the shared memory, so modifying its value does not
//        affect the shared memory.
// return
//  On success, return 0.
//  On error, return a negative number, representing the error's type:
//  E_CUSTOM_NAMETOOLONG: The provided name exceeds 99 characters long.
//  E_CUSTOM_INVALIDSHMNAME: The provided name isn't in the correct format.
//  E_CUSTOM_SHMOPEN: An error occurred in a shm_open() call; check errno.
//  E_CUSTOM_MMAP: An error occurred in a mmap() call; check errno.
//  E_CUSTOM_FTRUNCATE: An error occurred in a ftruncate() call; check errno.
int get_block_info(char* name, char* next_block_name_storage, char* block_hash_storage, BitcoinBlock* block_storage){
    if(strnlen(name,100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    int fd;
    BitcoinBlock* block;
    
    fd=shm_open(name, O_RDWR, 0666);
    if(fd==-1){
        return E_CUSTOM_SHMOPEN;
    }
    block=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(block==MAP_FAILED){
        return E_CUSTOM_MMAP;
    }
    if(next_block_name_storage!=NULL){
        strncpy(next_block_name_storage, block->next_block, 100);
    }
    if(block_hash_storage!=NULL){
        dsha(&(block->header), sizeof(BitcoinHeader), block_hash_storage);
    }
    if(block_storage!=NULL){
        memcpy(block_storage, block, sizeof(BitcoinBlock));
    }
    close(fd);
    //shm_unlink(name);
    munmap(block, sizeof(BitcoinBlock));
    return 0;
}

// A wrapper for get_block_info; obtain the next block's name of a block.
// params
//  *name: name of the shared memory of this block.
//  *next_name: pointer to store the next block's name.
// return
//  0 on success, a negative number if an error occurs.
int get_next_block_name(char* name, char* next_name){
    return get_block_info(name, next_name, NULL, NULL);
}

// A wrapper for get_block_info; obtain a block's header's hash.
// params
//  *name: name of the shared memory of this block.
//  *digest: pointer to store the block's hash.
// return
//  0 on success, a negative number if an error occurs.
int get_block_hash(char* name, char* digest){
    return get_block_info(name, NULL, digest, NULL);
}


// A wrapper for get_block_info; obtain a block's data in its entirety.
// params
//  *name: name of the shared memory of this block.
//  *block: pointer to store the block.
// return
//  0 on success, a negative number if an error occurs.
int get_block_data(char* name, BitcoinBlock* block){
    return get_block_info(name, NULL, NULL, block);
}

// Get a blockchain's info based on its genesis block's name.
// It's slightly hard to use, so it's recommended to use a wrapper for this
// function.
// params
//  *name: name of the shared memory for the genesis block.
//  *last_block_name_storage: if not NULL, copy the last block's name here.
//  *last_block_hash_storage:
//        if not NULL, copy the last block's header hash here.
//  *last_block_storage:
//        if not NULL, copy the entire block here. Note that this copy
//        is not mapped from the shared memory, so modifying its value does not
//        affect the shared memory.
// return
//  On success, return the number of blocks in the blockchain.
//  On error, return a negative number, representing the error's type:
//  E_CUSTOM_NAMETOOLONG: The provided name exceeds 99 characters long.
//  E_CUSTOM_INVALIDSHMNAME: The provided name isn't in the correct format.
//  E_CUSTOM_SHMOPEN: An error occurred in a shm_open() call; check errno.
//  E_CUSTOM_MMAP: An error occurred in a mmap() call; check errno.
//  E_CUSTOM_FTRUNCATE: An error occurred in a ftruncate() call; check errno.
int get_blockchain_info(char* genesis, char* last_block_name_storage, char* last_block_hash_storage, BitcoinBlock* last_block_storage){
    if(strnlen(genesis,100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(genesis)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    int fd;
    BitcoinBlock* block;
    char name[100];
    char name2[100];
    strcpy(name, genesis);
    int count=0;
    while(1){
        fd=shm_open(name, O_RDWR, 0666);
        if(fd==-1){
            return E_CUSTOM_SHMOPEN;
        }
        block=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
        if(block==MAP_FAILED){
            return E_CUSTOM_MMAP;
        }
        count++;
        if(block->next_block[0]==0){
            // no next block
            break;
        }
        if(strnlen(block->next_block, 100)>99){
            return E_CUSTOM_NAMETOOLONG;
        }
        if(!is_valid_block_shm_name(block->next_block)){
            return E_CUSTOM_INVALIDSHMNAME;
        }
        strcpy(name2, block->next_block);
        close(fd);
        //shm_unlink(name);
        munmap(block, sizeof(BitcoinBlock));
        strcpy(name, name2);
    }
    if(last_block_name_storage!=NULL){
        strcpy(last_block_name_storage, name);
    }
    if(last_block_hash_storage!=NULL){
        dsha(&(block->header), sizeof(BitcoinHeader), last_block_hash_storage);
    }
    if(last_block_storage!=NULL){
        memcpy(last_block_storage, block, sizeof(BitcoinBlock));
    }
    close(fd);
    //shm_unlink(name);
    munmap(block, sizeof(BitcoinBlock));
    return count;
}

// A wrapper for get_blockchain_info; obtain the number of blocks in the chain.
// params
//  *name: name of the shared memory of the genesis block.
// return
//  number of blocks on success, a negative number if an error occurs.
int get_blockchain_length(char* name){
    return get_blockchain_info(name, NULL, NULL, NULL);
}


// A wrapper for get_blockchain_info; obtain the last block's name.
// params
//  *name: name of the shared memory of the genesis block.
//  *last_name: pointer to store the last block's name.
// return
//  number of blocks on success, a negative number if an error occurs.
int get_last_block_name(char* name, char* last_name){
    return get_blockchain_info(name, last_name, NULL, NULL);
}

// A wrapper for get_blockchain_info; obtain the last block's header hash.
// params
//  *name: name of the shared memory of the genesis block.
//  *digest: pointer to store the last block's header hash.
// return
//  number of blocks on success, a negative number if an error occurs.
int get_last_block_hash(char* name, char* digest){
    return get_blockchain_info(name, NULL, digest, NULL);
}

// A wrapper for get_blockchain_info; obtain the last block's data in its entirety.
// params
//  *name: name of the shared memory of the genesis block.
//  *block: pointer to store the last block's data.
// return
//  number of blocks on success, a negative number if an error occurs.
int get_last_block_data(char* name, BitcoinBlock* block){
    return get_blockchain_info(name, NULL, NULL, block);
}

// Modify a blockchain's last block's `next_block` and another block's
// `previous_block`, so that they reference each other.
// Also calculates what the new block's shared memory name would be.
// params
//  *genesis: name of the shared memory of the genesis block.
//  *new_block: the new block to attach.
//  *new_block_name_storage: pointer to store the new block's shared memory name.
// return
//  0 on success, a negative number if an error occurs.
int attach_block(char* genesis, BitcoinBlock* new_block, char* new_block_name_storage){
    if(strnlen(genesis, 100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(genesis)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    char last_name[100];
    BitcoinBlock last_block;
    int rv;
    rv=get_blockchain_info(genesis, last_name, NULL, &last_block);
    if(rv<0){
        return rv;
    }
    if(!is_valid_block_shm_name(last_name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    char new_block_hash[32];
    char new_block_name[100];
    dsha(&(new_block->header), sizeof(BitcoinHeader), new_block_hash);
    construct_shm_name(new_block_hash, new_block_name);
    
    strcpy(last_block.next_block, new_block_name);
    strcpy(new_block->previous_block, last_name);
    strcpy(new_block_name_storage, new_block_name);
    
    int fd;
    BitcoinBlock* block;
    
    fd=shm_open(last_name, O_RDWR, 0666);
    if(fd==-1){
        return E_CUSTOM_SHMOPEN;
    }
    block=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(block==MAP_FAILED){
        return E_CUSTOM_MMAP;
    }
    memcpy(block, &last_block, sizeof(BitcoinBlock));
    close(fd);
    //shm_unlink(last_name);
    munmap(block, sizeof(BitcoinBlock));
    return 0;
}

// Unlink the shared memories used by a blockchain.
// Checks for error for the shared memory references. If one block has an issue
// (name is invalid, or failed to process its shared memory), the previous block
// is not unlinked, and that block's name is given to assist error handling.
// params
//  *name: name of the shared memory of the genesis block.
//  *name_failure: pointer to store the name of the not-unlinked block.
// return
//  0 on success, a negative number if an error occurs.
int unlink_shared_memories(char* name, char* name_failure){
    if(strnlen(name, 100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    char name1[100];
    char name2[100];
    char name3[100];
    int rv;
    strcpy(name1, name);
    // copying this pre-error so that I can immediately return without
    // potentially updating errno
    strcpy(name_failure, name1);
    rv=get_next_block_name(name1, name2);
    if(rv<0)return rv;
    if(name2[0]==0){
        // there is no second block
        shm_unlink(name1);
        return 0;
    }
    // there is supposedly a second block, but name1 can't be unlinked yet
    // has to verify name2 is actually a valid block
    while(1){
        rv=get_next_block_name(name2, name3);
        // N.B. if it fails here, it's either name2's fault or name2's shm's fault
        // it's definitely not name3's fault
        if(rv<0)return rv;
        // So name2 is good, unlinking name1
        shm_unlink(name1);
        if(name3[0]==0){
            // there is no name3, name2 is last block, unlink it
            shm_unlink(name2);
            return 0;
        }
        strcpy(name1, name2);
        strncpy(name2, name3, 100);
        strcpy(name_failure, name1);
    }
}

// Write a block in a shared memory.
// params
//  *name: name of the shared memory to write to.
//  *block: the block to write.
// return
//  Should return 0 on success, a negative number when an error occurs.
int write_block_in_shm(char* name, BitcoinBlock* block){
    if(strnlen(name, 100)>99){
        return E_CUSTOM_NAMETOOLONG;
    }
    if(!is_valid_block_shm_name(name)){
        return E_CUSTOM_INVALIDSHMNAME;
    }
    int fd=shm_open(name, O_CREAT|O_RDWR, 0666);
    if(fd==-1){
        return E_CUSTOM_SHMOPEN;
    }
    int rv=ftruncate(fd, sizeof(BitcoinBlock));
    if(rv==-1){
        return E_CUSTOM_FTRUNCATE;
    }
    BitcoinBlock* b=mmap(NULL, sizeof(BitcoinBlock), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if(b==MAP_FAILED){
        return E_CUSTOM_MMAP;
    }
    memcpy(b, block, sizeof(BitcoinBlock));
    
    close(fd);
    munmap(b, sizeof(BitcoinBlock));
    return 0;
}




