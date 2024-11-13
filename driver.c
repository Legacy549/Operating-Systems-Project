// driver.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transactionHandler.h"

#define MAX_INPUT_LENGTH 100

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
    int expectedNumUsers;
    int shmID;
    SharedMemorySegment *shmPtr;

    fscanf(file, "%d\n", &expectedNumUsers);
    initializeBankingSystem(expectedNumUsers, &shmID, &shmPtr);

    char line[MAX_INPUT_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        char accountID[20];
        char transactionType[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(line, "%s %s %d %s", accountID, transactionType, &amount, recipient);

        int accountIndex = findAccount(accounts, numAccounts, accountID);
        if (accountIndex == -1 && strcmp(transactionType, "Create") == 0) {
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
            write(accounts[accountIndex].writeFd, line, strlen(line) + 1);
            char response[MAX_INPUT_LENGTH];
            read(accounts[accountIndex].readFd, response, sizeof(response));
            printf("%s", response);
        } else {
            printf("Account %s not found\n", accountID);
        }
    }

    fclose(file);
    printf("\nTransaction Log:\n");
    for (int i = 0; i < shmPtr->transactionCount; i++) {
        printf("Transaction %d: %s, Account: %s, Amount: %d, Status: %s, Timestamp: %s\n",
               i + 1, shmPtr->transactions[i].transactionType, shmPtr->transactions[i].accountID,
               shmPtr->transactions[i].amount, shmPtr->transactions[i].status, shmPtr->transactions[i].timestamp);
    }

    destroyBankingSystem(shmID, shmPtr);
    return 0;
}
