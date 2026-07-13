/*
 * sandbox.c - User Space Malware Analysis Sandbox
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This sandbox runs untrusted binaries in an isolated child process
 * and monitors them using multiple threads. It enforces time limits
 * using SIGKILL and memory limits using RLIMIT_AS.
 *
 * Concepts demonstrated:
 * - fork() + execve() for process isolation
 * - pthreads for concurrent monitoring
 * - SIGKILL for forced termination
 * - RLIMIT_AS for memory limiting
 * - atomic_bool for thread-safe shared state
 * - /proc/[pid]/stat for external CPU monitoring
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>
#include <stdatomic.h>

#define TIME_LIMIT_SEC 5
#define MEMORY_LIMIT_MB 50

/* Shared state between monitoring threads */
static pid_t child_pid = -1;
static atomic_bool kill_flag = ATOMIC_VAR_INIT(0);

/*
 * log_message()
 * Prints timestamped log message.
 */
void log_message(const char *level, const char *msg) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] [%s] %s\n",
           t->tm_hour, t->tm_min, t->tm_sec, level, msg);
    fflush(stdout);
}

/*
 * timer_thread()
 * Monitors execution time. If child runs longer than TIME_LIMIT_SEC,
 * sets kill_flag and sends SIGKILL to child.
 * This enforcement is independent of the child's cooperation.
 */
void *timer_thread(void *arg) {
    char logbuf[256];

    snprintf(logbuf, sizeof(logbuf),
             "Timer thread started. Limit: %d seconds", TIME_LIMIT_SEC);
    log_message("TIMER", logbuf);

    sleep(TIME_LIMIT_SEC);

    /* Check if child is still running */
    if (!atomic_load(&kill_flag)) {
        atomic_store(&kill_flag, 1);
        snprintf(logbuf, sizeof(logbuf),
                 "Time limit exceeded! Killing child PID: %d", child_pid);
        log_message("TIMER", logbuf);
        kill(child_pid, SIGKILL);
        log_message("TIMER", "SIGKILL sent to child process.");
    }

    return NULL;
}

/*
 * cpu_monitor_thread()
 * Reads /proc/[pid]/stat to monitor CPU usage externally.
 * The child cannot hide or modify its own /proc entry.
 */
void *cpu_monitor_thread(void *arg) {
    char path[64];
    char logbuf[256];
    FILE *fp;
    long utime, stime;
    char dummy[32];
    int i;

    log_message("MONITOR", "CPU monitor thread started.");

    while (!atomic_load(&kill_flag)) {
        snprintf(path, sizeof(path), "/proc/%d/stat", child_pid);
        fp = fopen(path, "r");

        if (fp == NULL) {
            /* Process has ended */
            break;
        }

        /* Parse /proc/[pid]/stat - utime is field 14, stime is field 15 */
        fscanf(fp, "%s %s %s %s %s %s %s %s %s %s %s %s %s %ld %ld",
               dummy, dummy, dummy, dummy, dummy,
               dummy, dummy, dummy, dummy, dummy,
               dummy, dummy, dummy, &utime, &stime);
        fclose(fp);

        snprintf(logbuf, sizeof(logbuf),
                 "Child PID %d - CPU ticks: user=%ld kernel=%ld",
                 child_pid, utime, stime);
        log_message("MONITOR", logbuf);

        sleep(1);
    }

    log_message("MONITOR", "CPU monitor thread exiting.");
    return NULL;
}

/*
 * apply_resource_limits()
 * Applies resource limits to the child process before execve().
 * RLIMIT_AS limits total virtual memory the process can use.
 */
void apply_resource_limits() {
    struct rlimit mem_limit;
    char logbuf[256];

    mem_limit.rlim_cur = MEMORY_LIMIT_MB * 1024 * 1024;
    mem_limit.rlim_max = MEMORY_LIMIT_MB * 1024 * 1024;

    if (setrlimit(RLIMIT_AS, &mem_limit) < 0) {
        perror("[child] setrlimit failed");
    } else {
        snprintf(logbuf, sizeof(logbuf),
                 "Memory limit set to %dMB", MEMORY_LIMIT_MB);
        log_message("CHILD", logbuf);
    }
}

/*
 * run_sandboxed()
 * Main sandbox function. Forks child, starts monitoring threads,
 * waits for child to finish or be killed.
 */
int run_sandboxed(const char *binary_path) {
    pthread_t timer_tid, monitor_tid;
    int status;
    char logbuf[256];

    log_message("INFO", "Starting sandbox...");
    snprintf(logbuf, sizeof(logbuf), "Target binary: %s", binary_path);
    log_message("INFO", logbuf);
    snprintf(logbuf, sizeof(logbuf),
             "Limits: %ds time, %dMB memory",
             TIME_LIMIT_SEC, MEMORY_LIMIT_MB);
    log_message("INFO", logbuf);

    /* Fork child process */
    child_pid = fork();

    if (child_pid < 0) {
        perror("[sandbox] fork failed");
        return -1;
    }

    if (child_pid == 0) {
        /*
         * CHILD PROCESS
         * Apply resource limits before executing untrusted binary.
         * The child cannot remove these limits itself.
         */
        apply_resource_limits();

        char *argv[] = { (char*)binary_path, NULL };
        char *envp[] = { NULL };

        execve(binary_path, argv, envp);

        perror("[child] execve failed");
        exit(1);
    }

    /*
     * PARENT PROCESS - Sandbox Supervisor
     * Start monitoring threads to watch the child.
     */
    snprintf(logbuf, sizeof(logbuf),
             "Child process created with PID: %d", child_pid);
    log_message("SANDBOX", logbuf);

    /* Start timer thread */
    pthread_create(&timer_tid, NULL, timer_thread, NULL);

    /* Start CPU monitor thread */
    pthread_create(&monitor_tid, NULL, cpu_monitor_thread, NULL);

    /* Wait for child to finish */
    waitpid(child_pid, &status, 0);

    /* Signal threads that child is done */
    atomic_store(&kill_flag, 1);

    /* Wait for threads to finish */
    pthread_join(timer_tid, NULL);
    pthread_join(monitor_tid, NULL);

    /* Report how child exited */
    if (WIFEXITED(status)) {
        snprintf(logbuf, sizeof(logbuf),
                 "Child exited normally with code: %d",
                 WEXITSTATUS(status));
        log_message("SANDBOX", logbuf);
    } else if (WIFSIGNALED(status)) {
        snprintf(logbuf, sizeof(logbuf),
                 "Child was killed by signal: %d (%s)",
                 WTERMSIG(status), strsignal(WTERMSIG(status)));
        log_message("SANDBOX", logbuf);
    }

    log_message("INFO", "Sandbox session complete.");
    return status;
}

int main(int argc, char *argv[]) {
    printf("==========================================\n");
    printf("   User Space Malware Analysis Sandbox\n");
    printf("   ST5039CMD - Rijan Shrestha\n");
    printf("==========================================\n\n");

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary_to_sandbox>\n", argv[0]);
        fprintf(stderr, "Example: %s ./test_infinite\n", argv[0]);
        return 1;
    }

    run_sandboxed(argv[1]);
    return 0;
}
