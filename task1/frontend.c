/*
 * frontend.c - User Input Handler with UNIX Socket IPC
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This program handles user input and sends credentials to the backend
 * process over a UNIX domain socket. It never performs validation itself.
 *
 * Concepts demonstrated:
 * - termios: hide password input
 * - UNIX domain socket: secure local IPC
 * - explicit_bzero: secure memory clearing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define SOCKET_PATH "/tmp/auth.sock"

/*
 * Structure to hold credentials before sending over socket.
 * Using a struct ensures username and password are sent together
 * as one atomic message.
 */
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} Credentials;

/*
 * read_password()
 * Reads password from terminal without echoing it to screen.
 * Uses termios to temporarily disable terminal echo.
 */
void read_password(char *password, int max_len) {
    struct termios old_term, new_term;

    /* Save current terminal settings */
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;

    /* Disable echo */
    new_term.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    /* Read password */
    fgets(password, max_len, stdin);
    password[strcspn(password, "\n")] = '\0';

    /* Restore terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    printf("\n");
}

/*
 * connect_to_backend()
 * Creates a UNIX domain socket and connects to the backend process.
 * Returns socket file descriptor on success, -1 on failure.
 */
int connect_to_backend() {
    int sockfd;
    struct sockaddr_un addr;

    /* Create socket - AF_UNIX means UNIX domain (local only) */
    sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("[-] socket creation failed");
        return -1;
    }

    /* Set up socket address - point to the socket file */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    /* Connect to backend */
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

    /* Welcome banner */
    printf("========================================\n");
    printf("   Secure Authentication System\n");
    printf("   ST5039CMD - Rijan Shrestha\n");
    printf("========================================\n\n");

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
        fprintf(stderr, "[-] Could not reach backend. Is it running?\n");
        explicit_bzero(&creds, sizeof(creds));
        return 1;
    }

    /* Send credentials to backend as a struct */
    printf("[*] Sending credentials to backend...\n");
    if (send(sockfd, &creds, sizeof(creds), 0) < 0) {
        perror("[-] send failed");
        close(sockfd);
        explicit_bzero(&creds, sizeof(creds));
        return 1;
    }

    /* Wait for response from backend */
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

    /* Securely clear credentials from memory */
    explicit_bzero(&creds, sizeof(creds));
    printf("[*] Credentials cleared from memory.\n");

    return 0;
}
