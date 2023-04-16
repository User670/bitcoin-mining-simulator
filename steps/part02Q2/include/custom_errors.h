#ifndef CUSTOM_ERRORS_H
#define CUSTOM_ERRORS_H
// This is a custom implementation to imitate the purpose of errno.
// There is no errno variable - functions using this should either return
// an error code or take an int* parameter to store an error code.


#define E_CUSTOM_GENERIC -1
#define E_CUSTOM_SHMOPEN -7
#define E_CUSTOM_MMAP -8
#define E_CUSTOM_FTRUNCATE -9

#define E_CUSTOM_NAMETOOLONG -101
#define E_CUSTOM_INVALIDSHMNAME -102
void perror_custom(int errnum, char* prefix);

#endif
