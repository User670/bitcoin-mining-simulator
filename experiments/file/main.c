#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

int main (void) {
    /*int fd1, fd2, fd3;
    
    if ((fd1 = open ("./file1", O_RDWR|O_CREAT|O_TRUNC,0600)) == -1)
        return EXIT_FAILURE;
    if (write (fd1,"abcde", strlen ("abcde")) == -1) 
        return EXIT_FAILURE;
    if (fork () == 0) {
        if ((fd2 = open ("./file1", O_RDWR)) == -1)
            return EXIT_FAILURE;
        if (write (fd1,"123", strlen ("123")) == -1) 
            return EXIT_FAILURE;
        if (write (fd2,"45", strlen ("45")) == -1)
            return EXIT_FAILURE;
        close(fd2);
    } else {
        fd3 = dup(fd1);
        if (lseek (fd3,0,SEEK_SET) == -1) 
            return EXIT_FAILURE;
        if (write (fd3,"fg", strlen ("fg")) == -1) 
            return EXIT_FAILURE;
        if (write (fd1,"hi", strlen ("hi")) == -1) 
            return EXIT_FAILURE;
        wait (NULL);
        close (fd1);
        close(fd3);
    }
    return EXIT_SUCCESS;
    */
    
    /*
    int fd1=open("./relative.txt", O_RDWR|O_CREAT|O_TRUNC,0600);
    int fd2=open("../up-a-level.txt", O_RDWR|O_CREAT|O_TRUNC,0600);
    int fd3=open("direct.txt", O_RDWR|O_CREAT|O_TRUNC,0600);
    int fd4=open("./folder/direct.txt", O_RDWR|O_CREAT|O_TRUNC,0600);
    // above fails when ./folder doesn't already exist
    
    int s1=write(fd1, "lorem", 5);
    int s2=write(fd2, "lorem", 5);
    int s3=write(fd3, "lorem", 5);
    int s4=write(fd3, "lorem", 5);
    
    printf("%d %d %d %d\n", s1, s2, s3, s4);
    printf("%d %d %d %d\n", fd1, fd2, fd3, fd4);
    
    
    close(fd1);
    close(fd2);
    close(fd3);
    close(fd4);
    */
    int fd=open("number.txt", O_RDWR|O_CREAT|O_TRUNC,0600);
    printf("fd=%d\n", fd);
    int lorem=91825;
    write(fd, &lorem, sizeof(int));
    close(fd);
    
    int ipsum;
    fd=open("number.txt", O_RDWR|O_CREAT,0600);
    printf("fd=%d\n", fd);
    read(fd, (void*)&ipsum, sizeof(int));
    printf("%d\n", ipsum);
    close(fd);
    
}
