#include <custom_errors.h>
#include <stdio.h>

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
    if(prefix!=NULL && *prefix!='\0'){
        printf("%s: ",prefix);
    }
    switch(errnum){
        case E_CUSTOM_NOMEM:
            printf("No enough space in buffer\n");
            break;
        case E_CUSTOM_BADTREE:
            printf("Merkle tree malformatted\n");
            break;
        case E_CUSTOM_BADDATALEN:
            printf("Read zero or negative for data node length\n");
            break;
        case E_CUSTOM_BADTREELEN:
            printf("Read zero or negative for merkle tree length\n");
            break;
        case E_CUSTOM_EMPTYCHAIN:
            printf("Blockchain is empty\n");
            break;
        case E_CUSTOM_BADCHAINLEN:
            printf("Read negative for blockchain length\n");
            break;
    }
} 