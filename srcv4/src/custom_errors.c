#include <custom_errors.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

// Print an error message for a custom_errors error.
// If prefix is not NULL and is not '\0' (empty string), print prefix followed by
// a colon and a space.
// Then print a description of the error and a new line character.
// see also: standard library function `perror` from stdio.h.
// (tips: use `man perror` in your Linux console.)
// params
//  errnum: an error code. It should be obtained from a function that throws a
//          custom error.
//  *prefix: a string to go before the error description.
//           As with `perror`, it's best practice to include the name of the
//           function that threw the error.
// return
//  void
void perror_custom(int errnum, char* prefix){
    int errval=errno;
    if(prefix!=NULL && *prefix!='\0'){
        printf("%s: ",prefix);
    }
    switch(errnum){
        case E_CUSTOM_GENERIC:
            printf("Unspecified error\n");
            break;
        case E_CUSTOM_NAMETOOLONG:
            printf("Provided name too long\n");
            break;
        case E_CUSTOM_INVALIDSHMNAME:
            printf("Shared memory name is not in the correct format\n");
            break;
        case E_CUSTOM_SHMOPEN:
            printf("%s (raised by shm_open)\n",strerror(errval));
            break;
        case E_CUSTOM_MMAP:
            printf("%s (raised by mmap)\n",strerror(errval));
            break;
        case E_CUSTOM_FTRUNCATE:
            printf("%s (raised by ftruncate)\n",strerror(errval));
            break;
    }
} 