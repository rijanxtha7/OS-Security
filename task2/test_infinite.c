/*
 * test_infinite.c - Infinite loop test binary
 * Simulates a malicious program that never exits.
 * Used to test sandbox timeout enforcement.
 */
#include <stdio.h>

int main() {
    printf("[test_infinite] Starting infinite loop...\n");
    printf("[test_infinite] This program will never exit!\n");
    fflush(stdout);

    /* Infinite loop - simulates malware that won't stop */
    while(1) {
        /* doing nothing forever */
    }

    return 0;
}
