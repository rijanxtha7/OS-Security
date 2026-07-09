/*
 * frontend.c - User Input Handler with Process Isolation
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This program handles user input and launches the backend process
 * using fork() and execve(). It then sends credentials to the backend
 * over a UNIX domain socket.
 *
 * Concepts demonstrated:
 * - fork() + execve() to launch backend as separate process
 * - UNIX domain socket for secure IPC
 * - termios for hidden password input
 * - explicit_bzero for secure memory clearing
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define SOCKET_PATH "/tmp/auth.sock"
#define BACKEND_PATH "./backend"

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} Credentials;

/*
 * read_password()
 * Reads password without showing it on screen.
 */
void read_password(char *password, int max_len) {
    struct termios old_term, new_term;

    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    fgets(password, max_len, stdin);
    password[strcspn(password, "\n")] = '\0';

    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    printf("\n");
}

/*
 * launch_backend()
 * Uses fork() to create a child process, then execve() to replace
 * the child with the backend executable. The parent continues as
 * the frontend. This implements process isolation - two separate
 * executables running independently.
 * Returns the child PID on success, -1 on failure.
 */
pid_t launch_backend() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("[-] fork failed");
        return -1;
    }

    if (pid == 0) {
        /* We are the CHILD process */
        /* Replace this process image with backend executable */
        char *argv[] = { BACKEND_PATH, NULL };
        char *envp[] = { NULL };

        execve(BACKEND_PATH, argv, envp);

        /* If execve returns, it failed */
        perror("[-] execve failed");
        exit(1);
    }

    /* We are the PARENT process */
    printf("[*] Backend launched as child process (PID: %d)\n", pid);

    /* Wait for backend to start and create socket */
    sleep(1);

    return pid;
}

/*
 * connect_to_backend()
 * Creates UNIX domain socket and connects to backend.
 */
int connect_to_backend() {
    int sockfd;
    struct sockaddr_un addr;

    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-] socket creation failed");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[-] connection to backend failed");
        close(sockfd);
        return -1;
    }

    printf("[*] Connected to backend at %s\n", SOCKET_PATH);
    return sockfd;
}

int main() {
    char response[16];
    Credentials creds;
    int sockfd;
    pid_t backend_pid;

    /* Welcome banner */
    printf("========================================\n");
    printf("   Secure Authentication System\n");
    printf("   ST5039CMD - Rijan Shrestha\n");
    printf("========================================\n\n");

    /* Launch backend as a separate child process */
    printf("[*] Launching backend process...\n");
    backend_pid = launch_backend();
    if (backend_pid < 0) {
        fprintf(stderr, "[-] Failed to launch backend\n");
        return 1;
    }

    /* Get username */
    printf("Username: ");
    fgets(creds.username, MAX_USERNAME, stdin);
    creds.username[strcspn(creds.username, "\n")] = '\0';

    /* Get password without echo */
    printf("Password: ");
    read_password(creds.password, MAX_PASSWORD);

    printf("\n[*] Credentials received for user: %s\n", creds.username);

    /* Connect to backend over UNIX socket */
    sockfd = connect_to_backend();
    if (sockfd < 0) {
        fprintf(stderr, "[-] Could not reach backend.\n");
        explicit_bzero(&creds, sizeof(creds));
        return 1;
    }

    /* Send credentials */
    printf("[*] Sending credentials to backend...\n");
    if (send(sockfd, &creds, sizeof(creds), 0) < 0) {
        perror("[-] send failed");
        close(sockfd);
        explicit_bzero(&creds, sizeof(creds));
        return 1;
    }

    /* Receive response */
    memset(response, 0, sizeof(response));
    if (recv(sockfd, response, sizeof(response) - 1, 0) < 0) {
        perror("[-] recv failed");
        close(sockfd);
        explicit_bzero(&creds, sizeof(creds));
        return 1;
    }

    /* Display result */
    printf("[*] Backend response: %s\n", response);

    if (strcmp(response, "AUTH_OK") == 0) {
        printf("[+] Authentication SUCCESSFUL\n");
    } else {
        printf("[-] Authentication FAILED\n");
    }

    /* Clean up */
    close(sockfd);

    /* Wait for backend child process to finish */
    waitpid(backend_pid, NULL, 0);
    printf("[*] Backend process finished.\n");

    /* Securely clear credentials */
    explicit_bzero(&creds, sizeof(creds));
    printf("[*] Credentials cleared from memory.\n");

    return 0;
}
