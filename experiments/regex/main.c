//#include "something.h"
#include <stdio.h>
#include <regex.h>

int main(){
    regex_t re;
    regcomp(&re, "^/btcblock-[0-9a-f]{64}$", REG_NOSUB|REG_EXTENDED);
    if(r!=0){
        printf("Error on regcomp: %d\n", r);
    }
    return regexec(&re,"/btcblock-0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", 0, NULL, 0);
    printf("valid string returns %d\n",r);
    r=regexec(&re,"/btcblocks-0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef", 0, NULL, 0);
    printf("invalid string returns %d\n",r);
    printf("REG_NOMATCH is %d\n",REG_NOMATCH);
}