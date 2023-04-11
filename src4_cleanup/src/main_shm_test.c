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
#include <errno.h>

#include <sha2.h>
#include <bitcoin_utils.h>
#include <data_utils.h>
#include <debug_utils.h>

int my_open_shm(char* name){
    int fd;
    fd=shm_open(name, O_CREAT|O_EXCL|O_RDWR, 0666);
    if(fd==-1){
        if(errno==EEXIST){
            printf("Shared memory exists: %s\n", name);
        }else{
            printf("Shared memory opening failed: %s\n", name);
            perror("");
        }
        fd=shm_open(name, O_CREAT|O_RDWR, 0666);
    }else{
        printf("Shared memory opened: %s\n",name);
    }
    return fd;
}

int main(){
    int fd_blk=my_open_shm("/shm_wtf_blk");
    int fd_hash=my_open_shm("/shm_wtf_hash");
    int fd_data=my_open_shm("/shm_wtf_data");
    int fd_char=my_open_shm("/shm_wtf_char");
    printf("BitcoinBlockv3: %d\n",sizeof(BitcoinBlockv3));
    printf("MerkleTreeHashNode: %d\n",sizeof(MerkleTreeHashNode));
    printf("MerkleTreeDataNode: %d\n",sizeof(MerkleTreeDataNode));
    printf("BitcoinHeader: %d\n",sizeof(BitcoinHeader));
    printf("BitcoinBlockv2: %d\n",sizeof(BitcoinBlockv2));
    
    
    int opcode;
    printf("1 initialize and write, 2 read\n> ");
    scanf("%d",&opcode);
    if(opcode==1){
        ftruncate(fd_blk, sizeof(BitcoinBlockv3));
        BitcoinBlockv3* block=mmap(NULL, sizeof(BitcoinBlockv3), PROT_READ|PROT_WRITE, MAP_SHARED, fd_blk, 0);
        
        ftruncate(fd_hash, sizeof(MerkleTreeHashNode));
        MerkleTreeHashNode* hashnode=mmap(NULL, sizeof(MerkleTreeHashNode), PROT_READ|PROT_WRITE, MAP_SHARED, fd_hash, 0);
        
        block->header.difficulty=0x12345678;
        block->header.timestamp=time(NULL);
        block->merkle_tree=hashnode;
        
        dsha(&opcode, sizeof(int), block->merkle_tree->hash);
        //debug_print_header(block->header);
        //debug_print_hex_line(block->merkle_tree->hash, 32);
        printf("block: %lld\n",block);
        printf("tree : %lld\n",block->merkle_tree);
        printf("hash : %lld\n",hashnode);
    }else{
        BitcoinBlockv3* block=mmap(NULL, sizeof(BitcoinBlockv3), PROT_READ|PROT_WRITE, MAP_SHARED, fd_blk, 0);
        MerkleTreeHashNode* hashnode=mmap(NULL, sizeof(MerkleTreeHashNode), PROT_READ|PROT_WRITE, MAP_SHARED, fd_hash, 0);
        printf("block: %lld\n",block);
        printf("tree : %lld\n",block->merkle_tree);
        printf("hash : %lld\n",hashnode);
        //debug_print_header(block->header);
        //debug_print_hex_line(block->merkle_tree->hash, 32);
    }
    
    printf("...> ");
    scanf("%d",&opcode);
    
    shm_unlink("/shm_wtf_test");
}
