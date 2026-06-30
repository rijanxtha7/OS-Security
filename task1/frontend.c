/*
 * frontend.c - User Input Handler
 * ST5039CMD - Programming and Operating Systems
 * Author: Rijan Shrestha
 *
 * This program is the frontend component of the privilege separated
 * authentication system. It handles user input only and has no access
 * to password validation logic or sensitive authentication data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#define MAX_USERNAME 64
#define MAX_PASSWORD 64

/*
 * read_password()
 * Reads password without showing it on screen.
 * Uses termios to disable terminal echo temporarily.
 */
void read_password(char *password, int max_len) {
    struct termios old_term, new_term;

    /* Get current terminal settings */
    tcgetattr(STDIN_FILENO, &old_term);
    new_term = old_term;

    /* Turn off echo so password is not displayed */
    new_term.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);

    /* Read the password */
    fgets(password, max_len, stdin);

    /* Remove newline character if present */
    password[strcspn(password, "\n")] = '\0';

    /* Restore original terminal settings */
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
    printf("\n");
}

int main() {
    char username[MAX_USERNAME];
    char password[MAX_PASSWORD];

    /* Welcome banner */
    printf("========================================\n");
    printf("   Secure Authentication System\n");
    printf("   ST5039CMD - Rijan Shrestha\n");
    printf("========================================\n\n");

    /* Get username */
    printf("Username: ");
    fgets(username, MAX_USERNAME, stdin);
    username[strcspn(username, "\n")] = '\0';

    /* Get password without echo */
    printf("Password: ");
    read_password(password, MAX_PASSWORD);

    /* Confirm credentials received */
    printf("\n[*] Credentials received for user: %s\n", username);
    printf("[*] Contacting backend for validation...\n");

    /*
     * Socket communication to backend will be added in the next stage.
     */

    /* Securely clear password from memory */
    explicit_bzero(password, sizeof(password));
    printf("[*] Password cleared from memory.\n");

    return 0;
}
