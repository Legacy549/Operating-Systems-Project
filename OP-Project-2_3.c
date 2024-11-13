#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include "MonitorQueue.h"
#include "IPC_OS_Project.h"

#define MAX_ACCOUNTS 100
#define MAX_INPUT_LENGTH 100

typedef struct {
    char accountID[20];
    int balance;
    int open;
    pid_t pid;
    int readFd;
    int writeFd;
} Account;

// Declare the queue globally so it's accessible across functions
MonitorQueue queue;

int findAccount(Account accounts[], int numAccounts, char *accountID) {
    for (int i = 0; i < numAccounts; i++) {
        if (strcmp(accounts[i].accountID, accountID) == 0 && accounts[i].open) {
            return i;
        }
    }
    return -1;
}

void handleTransaction(int readFd, int writeFd, Account *account, Account accounts[], int numAccounts, SharedMemorySegment *shmPtr) {
    char buffer[MAX_INPUT_LENGTH];
    int isNewlyCreated = 1;  // Flag to check if it's the first time handling "Create"
    while (read(readFd, buffer, MAX_INPUT_LENGTH) > 0) {
        char transactionType[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(buffer, "%*s %s %d %s", transactionType, &amount, recipient);
        enterMonitorQueue(&queue, account->accountID);  // Synchronize entering queue

        if (strcmp(transactionType, "Create") == 0) {
            if (isNewlyCreated) {  // On first creation attempt, mark success
                account->balance = amount;
                account->open = 1;
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s created successfully with balance %d\n", account->accountID, account->balance);
                isNewlyCreated = 0;  // Set flag to avoid further checks
            } else {
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s already exists\n", account->accountID);
            }
        } else if (account->open) {
            if (strcmp(transactionType, "Deposit") == 0) {
                account->balance += amount;
                snprintf(buffer, MAX_INPUT_LENGTH, "Deposited %d into account %s\n", amount, account->accountID);
                logTransaction(shmPtr, "DEPOSIT", account->accountID, NULL, amount, "success");
            } else if (strcmp(transactionType, "Withdraw") == 0) {
                if (account->balance >= amount) {
                    account->balance -= amount;
                    snprintf(buffer, MAX_INPUT_LENGTH, "Withdrew %d from account %s\n", amount, account->accountID);
                    logTransaction(shmPtr, "WITHDRAW", account->accountID, NULL, amount, "success");
                } else {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Insufficient funds in account %s\n", account->accountID);
                    logTransaction(shmPtr, "WITHDRAW", account->accountID, NULL, amount, "failed");
                }
            } else if (strcmp(transactionType, "Inquiry") == 0) {
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s balance: %d\n", account->accountID, account->balance);
                logTransaction(shmPtr, "INQUIRY", account->accountID, NULL, 0, "success");
            } else if (strcmp(transactionType, "Close") == 0) {
                account->open = 0;
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s closed\n", account->accountID);
                logTransaction(shmPtr, "CLOSE", account->accountID, NULL, 0, "success");
            } else if (strcmp(transactionType, "Transfer") == 0) {
                int recipientIndex = findAccount(accounts, numAccounts, recipient);
                if (recipientIndex != -1 && accounts[recipientIndex].open && account->balance >= amount) {
                    account->balance -= amount;
                    accounts[recipientIndex].balance += amount;
                    snprintf(buffer, MAX_INPUT_LENGTH, "Transferred %d from %s to %s\n", amount, account->accountID, recipient);
                    logTransaction(shmPtr, "TRANSFER", account->accountID, recipient, amount, "success");
                } else {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Transfer failed from %s to %s\n", account->accountID, recipient);
                    logTransaction(shmPtr, "TRANSFER", account->accountID, recipient, amount, "failed");
                }
            } else {
                snprintf(buffer, MAX_INPUT_LENGTH, "Invalid operation\n");
            }
        } else {
            snprintf(buffer, MAX_INPUT_LENGTH, "Invalid operation or account is closed\n");
        }

        write(writeFd, buffer, strlen(buffer) + 1);
        exitMonitorQueue(&queue, account->accountID);  // Synchronize exiting queue
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

    int numAccounts = 0;
    Account accounts[MAX_ACCOUNTS];

    // Read the number of users and initialize the queue
    int expectedNumUsers;
    fscanf(file, "%d\n", &expectedNumUsers);
    initializeMonitorQueue(&queue, expectedNumUsers);

    // Set up shared memory
    int shmID;
    SharedMemorySegment *shmPtr;
    initSharedMemory(&shmID, &shmPtr);

    char line[MAX_INPUT_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char accountID[20];
        char transactionType[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(line, "%s %s %d %s", accountID, transactionType, &amount, recipient);

        int accountIndex = findAccount(accounts, numAccounts, accountID);
        if (accountIndex == -1 && strcmp(transactionType, "Create") == 0) {
            // New account creation
            accountIndex = numAccounts++;
            strcpy(accounts[accountIndex].accountID, accountID);
            accounts[accountIndex].balance = 0;
            accounts[accountIndex].open = 1;

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
                close(pipes[0][1]);
                close(pipes[1][0]);
                handleTransaction(pipes[0][0], pipes[1][1], &accounts[accountIndex], accounts, numAccounts, shmPtr);
                close(pipes[0][0]);
                close(pipes[1][1]);
                exit(0);
            } else { // Parent process
                accounts[accountIndex].pid = pid;
                accounts[accountIndex].readFd = pipes[1][0];
                accounts[accountIndex].writeFd = pipes[0][1];
                close(pipes[0][0]);
                close(pipes[1][1]);
            }
        }

        if (accountIndex != -1) {
            // Send transaction to the appropriate account's process
            write(accounts[accountIndex].writeFd, line, strlen(line) + 1);

            // Read response from child
            char response[MAX_INPUT_LENGTH];
            read(accounts[accountIndex].readFd, response, sizeof(response));
            printf("%s", response);
        } else {
            printf("Account %s not found\n", accountID);
        }
    }

    fclose(file);

    // Print shared memory transaction log after all transactions
    printf("\nTransaction Log:\n");
    for (int i = 0; i < shmPtr->transactionCount; i++) {
        printf("Transaction %d: %s, Account: %s, Amount: %d, Status: %s, Timestamp: %s\n",
               i + 1, shmPtr->transactions[i].transactionType, shmPtr->transactions[i].accountID,
               shmPtr->transactions[i].amount, shmPtr->transactions[i].status, shmPtr->transactions[i].timestamp);
    }

    // Cleanup
    destroyMonitorQueue(&queue);
    destroySharedMemory(shmID, shmPtr);
    return 0;
}

