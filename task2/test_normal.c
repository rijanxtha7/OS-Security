/*
 * test_normal.c - Normal test binary for sandbox
 * Simulates a normal program that runs and exits cleanly.
 */
#include <stdio.h>
#include <unistd.h>

int main() {
    printf("[test_normal] Starting...\n");
    printf("[test_normal] Doing some work...\n");
    sleep(2);
    printf("[test_normal] Done. Exiting normally.\n");
    return 0;
}
