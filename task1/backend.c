/*
 * backend.c - Privilege Separated Password Validation Service
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This program receives credentials from the frontend over a UNIX
 * domain socket, drops root privileges permanently using setresuid(),
 * then validates the credentials.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>

#define MAX_USERNAME 64
#define MAX_PASSWORD 64
#define SOCKET_PATH "/tmp/auth.sock"
#define NOBODY_UID 65534
#define NOBODY_GID 65534

typedef struct {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];
} Credentials;

int drop_privileges() {
    printf("[backend] Dropping privileges to nobody (UID %d)...\n", NOBODY_UID);

    if (setresgid(NOBODY_GID, NOBODY_GID, NOBODY_GID) < 0) {
        perror("[backend] setresgid failed");
        return -1;
    }

    if (setresuid(NOBODY_UID, NOBODY_UID, NOBODY_UID) < 0) {
        perror("[backend] setresuid failed");
        return -1;
    }

    if (geteuid() != NOBODY_UID) {
        fprintf(stderr, "[backend] CRITICAL: privilege drop failed!\n");
        return -1;
    }

    printf("[backend] Privileges dropped successfully.\n");
    printf("[backend] Running as UID: %d (effective: %d)\n",
           getuid(), geteuid());

    return 0;
}

int verify_peer(int client_fd) {
    struct ucred peer_cred;
    socklen_t len = sizeof(peer_cred);

    if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED,
                   &peer_cred, &len) < 0) {
        perror("[backend] getsockopt SO_PEERCRED failed");
        return 0;
    }

    printf("[backend] Connection from PID: %d, UID: %d\n",
           peer_cred.pid, peer_cred.uid);

    if (peer_cred.uid != getuid() && peer_cred.uid != 0) {
        fprintf(stderr, "[backend] Rejected connection from UID %d\n",
                peer_cred.uid);
        return 0;
    }

    return 1;
}

int validate_credentials(const char *username, const char *password) {
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
    printf("[backend] Initial UID: %d (effective: %d)\n",
           getuid(), geteuid());

    unlink(SOCKET_PATH);

    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[backend] socket creation failed");
        exit(1);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("[backend] bind failed");
        close(server_fd);
        exit(1);
    }

    if (listen(server_fd, 5) < 0) {
        perror("[backend] listen failed");
        close(server_fd);
        exit(1);
    }

    printf("[backend] Socket bound to %s\n", SOCKET_PATH);

    if (drop_privileges() < 0) {
        fprintf(stderr, "[backend] Failed to drop privileges. Exiting.\n");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(1);
    }

    printf("[backend] Waiting for frontend connection...\n");

    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd < 0) {
        perror("[backend] accept failed");
        close(server_fd);
        exit(1);
    }

    if (!verify_peer(client_fd)) {
        fprintf(stderr, "[backend] Peer verification failed. Rejecting.\n");
        close(client_fd);
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(1);
    }

    printf("[backend] Frontend connected and verified.\n");

    memset(&creds, 0, sizeof(creds));
    if (recv(client_fd, &creds, sizeof(creds), 0) < 0) {
        perror("[backend] recv failed");
        close(client_fd);
        close(server_fd);
        exit(1);
    }

    printf("[backend] Received credentials for user: %s\n", creds.username);

    if (validate_credentials(creds.username, creds.password)) {
        printf("[backend] Authentication SUCCESSFUL\n");
        strncpy(response, "AUTH_OK", sizeof(response));
    } else {
        printf("[backend] Authentication FAILED\n");
        strncpy(response, "AUTH_FAIL", sizeof(response));
    }

    send(client_fd, response, sizeof(response), 0);

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH);

    printf("[backend] Done. Exiting.\n");
    return 0;
}
