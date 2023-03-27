#include <time.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    time_t timestamp = time(NULL); // Unix timestamp for April 25, 2022, 00:00:00 UTC
    struct tm *timeinfo = localtime(&timestamp);
    char* buffer=malloc(81);
    strftime(buffer, 80, "%Y-%m-%d %H:%M:%S (%Z)", timeinfo);
    printf("Human-readable time: %s\n", buffer);
    return 0;
}