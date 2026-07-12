/*
 * sandbox.c - User Space Malware Analysis Sandbox
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This program creates a sandboxed environment to run untrusted
 * binaries safely. The parent process supervises the child from
 * outside using fork(), execve(), and waitpid().
 *
 * Concepts demonstrated:
 * - fork() + execve() for process isolation
 * - waitpid() for external supervision
 * - Parent-child process model
 * - Signal detection from child exit status
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

/*
 * log_message()
 * Prints a timestamped log message to stdout.
 */
void log_message(const char *level, const char *msg) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    printf("[%02d:%02d:%02d] [%s] %s\n",
           t->tm_hour, t->tm_min, t->tm_sec, level, msg);
}

/*
 * run_sandboxed()
 * Forks a child process and executes the untrusted binary inside it.
 * The parent monitors the child using waitpid().
 * Returns the exit status of the child.
 */
int run_sandboxed(const char *binary_path) {
    pid_t child_pid;
    int status;
    char logbuf[256];

    log_message("INFO", "Starting sandbox...");
    snprintf(logbuf, sizeof(logbuf), "Target binary: %s", binary_path);
    log_message("INFO", logbuf);

    /* fork() creates a copy of this process */
    child_pid = fork();

    if (child_pid < 0) {
        perror("[sandbox] fork failed");
        return -1;
    }

    if (child_pid == 0) {
        /*
         * CHILD PROCESS
         * We are now in the child. Replace this process with
         * the untrusted binary using execve().
         * The child has no knowledge of being monitored.
         */
        char *argv[] = { (char*)binary_path, NULL };
        char *envp[] = { NULL };

        log_message("INFO", "Child process started, executing binary...");

        execve(binary_path, argv, envp);

        /* If execve returns, something went wrong */
        perror("[child] execve failed");
        exit(1);
    }

    /*
     * PARENT PROCESS
     * We are the sandbox supervisor. We monitor the child
     * from outside without being affected by it.
     */
    snprintf(logbuf, sizeof(logbuf),
             "Child process created with PID: %d", child_pid);
    log_message("SANDBOX", logbuf);
    log_message("SANDBOX", "Monitoring child process...");

    /* Wait for child to finish and collect exit status */
    waitpid(child_pid, &status, 0);

    /* Analyse how the child exited */
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
    } else {
        log_message("SANDBOX", "Child exited with unknown status");
    }

    log_message("INFO", "Sandbox session complete.");
    return status;
}

int main(int argc, char *argv[]) {
    printf("==========================================\n");
    printf("   User Space Malware Analysis Sandbox\n");
    printf("   ST5039CMD - Rijan Shrestha\n");
    printf("==========================================\n\n");

    /* Check arguments */
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <binary_to_sandbox>\n", argv[0]);
        fprintf(stderr, "Example: %s ./test_binary\n", argv[0]);
        return 1;
    }

    /* Run the untrusted binary inside the sandbox */
    run_sandboxed(argv[1]);

    return 0;
}
