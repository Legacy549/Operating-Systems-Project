// transactionHandler.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transactionHandler.h"
#include "MonitorQueue.h"
#include "IPC_OS_Project.h"

MonitorQueue queue; // Monitor queue for synchronization

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

void initializeBankingSystem(int expectedNumUsers, int *shmID, SharedMemorySegment **shmPtr) {
    initializeMonitorQueue(&queue, expectedNumUsers);
    initSharedMemory(shmID, shmPtr);  // Initialize shared memory
}

void destroyBankingSystem(int shmID, SharedMemorySegment *shmPtr) {
    destroyMonitorQueue(&queue);
    destroySharedMemory(shmID, shmPtr);
}
