#include <custom_errors.h>
#include <stdio.h>

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