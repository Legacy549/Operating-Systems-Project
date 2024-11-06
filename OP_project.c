#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#define MAX_ACCOUNTS 100
#define MAX_INPUT_LENGTH 100

typedef struct {
    char account_id[20];
    int balance;
    int open;
    pid_t pid;
    int read_fd;
    int write_fd;
    pthread_mutex_t lock; // Mutex for locking the account
} Account;

// Function to find an account by ID
int find_account(Account accounts[], int num_accounts, char *account_id) {
    for (int i = 0; i < num_accounts; i++) {
        if (strcmp(accounts[i].account_id, account_id) == 0 && accounts[i].open) {
            return i;
        }
    }
    return -1;
}

// Handle transactions for the account
void handle_transaction(int read_fd, int write_fd, Account *account) {
    char buffer[MAX_INPUT_LENGTH];

    while (read(read_fd, buffer, MAX_INPUT_LENGTH) > 0) {
        char transaction_type[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(buffer, "%*s %s %d %s", transaction_type, &amount, recipient);

        // Lock the account before processing the transaction
        pthread_mutex_lock(&account->lock);

        if (strcmp(transaction_type, "Create") == 0) {
            if (account->open) {
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s already exists\n", account->account_id);
            } else {
                account->balance = amount;
                account->open = 1;
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s created with balance %d\n", account->account_id, account->balance);
            }
        } else if (account->open) {
            if (strcmp(transaction_type, "Deposit") == 0) {
                account->balance += amount;
                snprintf(buffer, MAX_INPUT_LENGTH, "Deposited %d into account %s\n", amount, account->account_id);
            } else if (strcmp(transaction_type, "Withdraw") == 0) {
                if (account->balance >= amount) {
                    account->balance -= amount;
                    snprintf(buffer, MAX_INPUT_LENGTH, "Withdrew %d from account %s\n", amount, account->account_id);
                } else {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Insufficient funds in account %s\n", account->account_id);
                }
            } else if (strcmp(transaction_type, "Inquiry") == 0) {
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s balance: %d\n", account->account_id, account->balance);
            } else if (strcmp(transaction_type, "Close") == 0) {
                account->open = 0;
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s closed\n", account->account_id);
            } else {
                snprintf(buffer, MAX_INPUT_LENGTH, "Invalid operation\n");
            }
        } else {
            snprintf(buffer, MAX_INPUT_LENGTH, "Invalid operation or account is closed\n");
        }

        // Unlock the account after processing
        pthread_mutex_unlock(&account->lock);
        
        write(write_fd, buffer, strlen(buffer) + 1);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
        return 1;
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    int num_accounts = 0;
    Account accounts[MAX_ACCOUNTS];

    // Initialize mutexes for each account
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_init(&accounts[i].lock, NULL);
    }

    // Read the first line to get the number of users (accounts)
    int expected_num_users;
    fscanf(file, "%d\n", &expected_num_users);

    char line[MAX_INPUT_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char account_id[20];
        char transaction_type[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(line, "%s %s %d %s", account_id, transaction_type, &amount, recipient);

        int account_index = find_account(accounts, num_accounts, account_id);
        if (account_index == -1 && strcmp(transaction_type, "Create") == 0) {
            // New account creation - initialize process and pipes
            account_index = num_accounts++;
            strcpy(accounts[account_index].account_id, account_id);
            accounts[account_index].balance = 0;
            accounts[account_index].open = 1;

            int pipes[2][2];
            if (pipe(pipes[0]) == -1 || pipe(pipes[1]) == -1) {
                perror("pipe failed");
                exit(1);
            }

            pid_t pid = fork();
            if (pid < 0) {
                perror("fork failed");
                exit(1);
            } else if (pid == 0) { // Child process
                close(pipes[0][1]); // Close write end of parent-to-child pipe
                close(pipes[1][0]); // Close read end of child-to-parent pipe
                handle_transaction(pipes[0][0], pipes[1][1], &accounts[account_index]);
                close(pipes[0][0]);
                close(pipes[1][1]);
                exit(0);
            } else { // Parent process
                accounts[account_index].pid = pid;
                accounts[account_index].read_fd = pipes[1][0];
                accounts[account_index].write_fd = pipes[0][1];
                close(pipes[0][0]);
                close(pipes[1][1]);
            }
        }

        if (account_index != -1) {
            // Send transaction to the appropriate account's process
            write(accounts[account_index].write_fd, line, strlen(line) + 1);

            // Read response from child
            char response[MAX_INPUT_LENGTH];
            read(accounts[account_index].read_fd, response, sizeof(response));
            printf("%s", response);
        } else {
            printf("Account %s not found\n", account_id);
        }
    }

    fclose(file);

    // Close all pipes and wait for child processes to finish
    for (int i = 0; i < num_accounts; i++) {
        close(accounts[i].write_fd);
        close(accounts[i].read_fd);
        waitpid(accounts[i].pid, NULL, 0);
    }

    // Destroy mutexes for each account
    for (int i = 0; i < MAX_ACCOUNTS; i++) {
        pthread_mutex_destroy(&accounts[i].lock);
    }

    return 0;
}
 


