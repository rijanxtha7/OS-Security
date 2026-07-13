/*
 * test_memory.c - Memory hog test binary
 * Simulates a malicious program that allocates huge amounts of memory.
 * Used to test sandbox memory limit enforcement.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("[test_memory] Starting memory allocation attack...\n");
    fflush(stdout);

    int count = 0;
    while(1) {
        /* Try to allocate 10MB at a time */
        char *block = malloc(10 * 1024 * 1024);
        if (block == NULL) {
            printf("[test_memory] malloc failed at iteration %d\n", count);
            break;
        }
        memset(block, 'A', 10 * 1024 * 1024);
        count++;
        printf("[test_memory] Allocated %dMB so far...\n", count * 10);
        fflush(stdout);
    }
    return 0;
}
