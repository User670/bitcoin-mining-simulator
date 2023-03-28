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

int main(){
    
    int fd;
    
    fd=open("1.txt", O_CREAT|O_EXCL|O_RDWR, 0666);
    if(fd==-1){
        if(errno==EEXIST){
            printf("EEXIST: the shared memory already exists.\n");
        }else{
            perror("other errno");
        }
        fd=open("1.txt", O_CREAT|O_RDWR, 0666);
    }else{
        printf("You are the first one to open this shared memory!\n");
    }
    
    ftruncate(fd, sizeof(int));
    int* sd=mmap(NULL, sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    
    int opcode;
    
    while(1){
        printf("1 to write, 2 to read, 3 to exit > ");
        scanf("%d", &opcode);
        switch(opcode){
            case 1:
                printf("Value to write > ");
                scanf("%d", sd);
                break;
            case 2:
                printf("Read value %d\n", *sd);
                break;
            case 3:
                close(fd);
                shm_unlink("/minejobs");
                exit(0);
        }
    }
}