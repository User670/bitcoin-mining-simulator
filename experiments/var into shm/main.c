#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

int main(){
    /*int fd;
    
    fd=shm_open("/minejobs", O_CREAT|O_EXCL|O_RDWR, 0666);
    if(fd==-1){
        if(errno==EEXIST){
            printf("EEXIST: the shared memory already exists.\n");
        }else{
            printf("other errno: some other reasons prevented the shared memory from opening.\n");
            perror(NULL);
        }
        fd=shm_open("/minejobs", O_CREAT|O_RDWR, 0666);
    }else{
        printf("You are the first one to open this shared memory!\n");
    }
    
    int shmsize=16;
    ftruncate(fd, shmsize);
    int* a=mmap(NULL, shmsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    *(a+1)=0x12345678;
    printf("%x\n",*(a+1));
    
    *(a+3)=0x23456789;
    printf("%x\n",*(a+3));
    */
    int fd=open("lorem.bin", O_RDWR|O_CREAT|O_TRUNC,0600);
    ftruncate(fd, sizeof(int)*2);
    int* a=mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    
    int opcode;
    int offset=0;
    int aaaa=0x40404040;
    while(1){
        printf("0-break, 1-write next\n");
        printf("> ");
        scanf("%d",&opcode);
        switch(opcode){
            case 0:
                exit(0);
                break;
            case 1:
                write(fd, &aaaa, sizeof(int));
                break;
        }
    }
    
    
    
    
    
}