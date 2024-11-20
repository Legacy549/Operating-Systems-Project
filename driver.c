// driver.c

//Author Name: Karson Younger
//Email: karson.younger@okstate.edu
//Date: 11/17/24
//Group: Group F
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

// Function to acquire a shared lock with timeout, takes in the resource that needs to be locked and the time out time.
int acquireLockWithTimeout(int lockFile, int timeoutSeconds) {
    int elapsed = 0;
    while (elapsed < timeoutSeconds) {
         // Shared lock (non-blocking)
        if (flock(lockFile, LOCK_SH | LOCK_NB) == 0) { 
            return 0; 
        } else if (errno == EAGAIN) {
            //retry after 1 second
            sleep(1);  
            elapsed++;
            printf("Lock not available, retrying...\n");
        } else {
            //failure Case
            perror("Error acquiring lock");
            return -1; 
        }
    }
    // timeout
    return -1;  
}

// our read function this takes in the file and number of accounts as well as shared memory pointers to properly delegate transactions
void readTransactions(FILE *file, int *numAccounts, Account accounts[MAX_ACCOUNTS], SharedMemorySegment *shmPtr, int expectedNumUsers) {
    char line[MAX_INPUT_LENGTH];
    
    while (fgets(line, sizeof(line), file)) {
        char accountID[20];
        char transactionType[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(line, "%s %s %d %s", accountID, transactionType, &amount, recipient);

        int accountIndex = findAccount(shmPtr, accountID);

        // Special Create hnadler
        if (accountIndex == -1 && strcmp(transactionType, "Create") == 0) {
            if (*numAccounts >= expectedNumUsers) {
                printf("Error: Exceeded maximum number of accounts (%d).\n", expectedNumUsers);
                // Log a failed 'Create' transaction
                logTransaction(shmPtr, "CREATE", accountID, "", amount, "failed");
                printf("Logging failed 'Create' transaction.\n");
            } else {
                // Create new account
                accountIndex = (*numAccounts)++;
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

                    //release lock
                    flock(lockFile, LOCK_UN);
                    close(lockFile);

                    close(pipes[0][0]);
                    close(pipes[1][1]);
                    exit(0);
                } else {
                    accounts[accountIndex].pid = pid;
                    accounts[accountIndex].readFd = pipes[1][0];
                    accounts[accountIndex].writeFd = pipes[0][1];
                    close(pipes[0][0]);
                    close(pipes[1][1]);
                }
            }
        }

        // this makes sure you cant create an account twice
        if (accountIndex != -1) {
            write(accounts[accountIndex].writeFd, line, strlen(line) + 1);
            char response[MAX_INPUT_LENGTH];
            read(accounts[accountIndex].readFd, response, sizeof(response));
            printf("%s", response);
        } else {
            printf("Account %s not found\n", accountID);
            logTransaction(shmPtr, transactionType, accountID, "", amount, "failed");
        }
    }
}
//main function
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

    // Now call the read function to process the file
    readTransactions(file, &numAccounts, accounts, shmPtr, expectedNumUsers);

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
