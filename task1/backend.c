/*
 * backend.c - Password Validation Service
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This program is the backend component of the privilege separated
 * authentication system. It receives credentials from the frontend
 * over a UNIX domain socket and validates them.
 *
 * Concepts demonstrated:
 * - UNIX domain socket server
 * - Receiving credentials from frontend
 * - Privilege separation (setresuid will be added next stage)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define SOCKET_PATH "/tmp/auth.sock"

/*
 * Structure must match exactly what frontend sends.
 * Both programs use the same struct layout.
 */
typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} Credentials;

/*
 * validate_credentials()
 * Checks username and password against known values.
 * In a real system this would check a hashed password database.
 * Returns 1 if valid, 0 if invalid.
 */
int validate_credentials(const char *username, const char *password) {
    /* Hardcoded credentials for demonstration purposes */
    const char *valid_user = "admin";
    const char *valid_pass = "secret123";

    if (strcmp(username, valid_user) == 0 &&
        strcmp(password, valid_pass) == 0) {
        return 1;
    }
    return 0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un addr;
    Credentials creds;
    char response[16];

    printf("[backend] Starting authentication service...\n");

    /* Remove old socket file if it exists */
    unlink(SOCKET_PATH);

    /* Create UNIX domain socket */
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[backend] socket creation failed");
        exit(1);
    }

    /* Set up socket address */
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    /* Bind socket to file path */
    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[backend] bind failed");
        close(server_fd);
        exit(1);
    }

    /* Listen for incoming connections - max 5 queued */
    if (listen(server_fd, 5) < 0) {
        perror("[backend] listen failed");
        close(server_fd);
        exit(1);
    }

    printf("[backend] Listening on %s\n", SOCKET_PATH);
    printf("[backend] Waiting for frontend connection...\n");

    /* Accept one connection from frontend */
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("[backend] accept failed");
        close(server_fd);
        exit(1);
    }

    printf("[backend] Frontend connected.\n");

    /* Receive credentials struct from frontend */
    memset(&creds, 0, sizeof(creds));
    if (recv(client_fd, &creds, sizeof(creds), 0) < 0) {
        perror("[backend] recv failed");
        close(client_fd);
        close(server_fd);
        exit(1);
    }

    printf("[backend] Received credentials for user: %s\n", creds.username);

    /* Validate credentials */
    if (validate_credentials(creds.username, creds.password)) {
        printf("[backend] Authentication SUCCESSFUL\n");
        strncpy(response, "AUTH_OK", sizeof(response));
    } else {
        printf("[backend] Authentication FAILED\n");
        strncpy(response, "AUTH_FAIL", sizeof(response));
    }

    /* Send response back to frontend */
    send(client_fd, response, sizeof(response), 0);

    /* Clean up */
    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    printf("[backend] Connection closed. Exiting.\n");
    return 0;
}
