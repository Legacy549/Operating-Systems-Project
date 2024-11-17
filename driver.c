// driver.c

//Author Name: Karson Younger
//Email: karson.younger@okstate.edu
//Date: 11/17/24
//Program Description: This is the main program driver and is repsonible for reading the transactions and forking accounts

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>  
#include <errno.h>  
#include <sys/file.h>  
#include "transactionHandler.h"


#define MAX_INPUT_LENGTH 100

// Function to acquire a shared lock with timeout
int acquireLockWithTimeout(int lockFile, int timeoutSeconds) {
    int elapsed = 0;
    while (elapsed < timeoutSeconds) {
        if (flock(lockFile, LOCK_SH | LOCK_NB) == 0) {  
            return 0; 
        } else if (errno == EAGAIN) {
            sleep(1); 
            elapsed++;
            printf("Lock not available, retrying...\n");
        } else {
            perror("Error acquiring lock");
            return -1; 
        }
    }
    return -1;  
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

        int accountIndex = findAccount(shmPtr, accountID);

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
                // Child process
            } else if (pid == 0) {  
                close(pipes[0][1]);
                close(pipes[1][0]);

                // Ensure locks are acquired with timeout
                int lockFile = open("/tmp/account_lock", O_CREAT | O_RDWR, 0666);
                if (lockFile == -1) {
                    perror("Failed to open lock file");
                    exit(1);
                }

                // Attempt to acquire the shared lock
                if (acquireLockWithTimeout(lockFile, 5) != 0) {
                    perror("Failed to acquire lock, exiting child process");
                    close(lockFile);
                    exit(1);
                }

                handleTransaction(pipes[0][0], pipes[1][1], &accounts[accountIndex], shmPtr);


                // Release lock
                flock(lockFile, LOCK_UN);
                close(lockFile);

                close(pipes[0][0]);
                close(pipes[1][1]);
                exit(0);
            } else {  // Parent process
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
            logTransaction(shmPtr, "CREATE", accountID, NULL, 0, "failed");
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
