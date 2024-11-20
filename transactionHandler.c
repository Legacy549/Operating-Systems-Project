// transactionHandler.c

// Author Name: Karson Younger
// Email: karson.younger@okstate.edu
// Date: 11/17/24
//Group: Group F
// Program Description: This handles all accounts and makes sure an account isnt create twice.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "transactionHandler.h"
#include "MonitorQueue.h"
#include "IPC_OS_Project.h"

MonitorQueue queue; // Monitor queue for synchronization

// Find an account by ID in shared memory, takes in the shared memory poitner and account ID of the target acount and returns its index if found. 
int findAccount(SharedMemorySegment *shmPtr, const char *accountID)
{
    for (int i = 0; i < MAX_ACCOUNTS; i++)
    {
        if (shmPtr->accounts[i].open && strcmp(shmPtr->accounts[i].accountID, accountID) == 0)
        {
            return i; // Return the index of the account if found
        }
    }
    return -1; // Return -1 if account is not found
}

// Handle transactions (Create, Deposit, Withdraw, Transfer, etc.) this will take in the pipeline variables adn account pointers as well as shared memory poiters and execute transactions. 
void handleTransaction(int readFd, int writeFd, Account *account, SharedMemorySegment *shmPtr)
{
    char buffer[MAX_INPUT_LENGTH];
    int isNewlyCreated = 1; // Flag to check if it's the first time handling "Create"
    while (read(readFd, buffer, MAX_INPUT_LENGTH) > 0)
    {
        char transactionType[20];
        int amount = 0;
        char recipient[20] = "";

        sscanf(buffer, "%*s %s %d %s", transactionType, &amount, recipient);
        enterMonitorQueue(&queue, account->accountID); // Synchronize entering queue

        if (strcmp(transactionType, "Create") == 0)
        {
            if (isNewlyCreated)
            {
                // Check if the account already exists
                int accountIndex = findAccount(shmPtr, account->accountID);
                if (accountIndex == -1)
                {
                    // Find first available slot in shared memory
                    for (int i = 0; i < MAX_ACCOUNTS; i++)
                    {
                        if (!shmPtr->accounts[i].open)
                        {
                            // Create the account in shared memory
                            strncpy(shmPtr->accounts[i].accountID, account->accountID, sizeof(shmPtr->accounts[i].accountID) - 1);
                            shmPtr->accounts[i].balance = amount;
                            // Mark account as open
                            shmPtr->accounts[i].open = 1; 

                            // Synchronize the local account balance with shared memory
                            account->balance = amount; 
                            account->open = 1;        

                            snprintf(buffer, MAX_INPUT_LENGTH, "Account %s created successfully with balance %d\n", account->accountID, amount);

                            // Log the transaction directly to logTransaction
                            logTransaction(shmPtr, "CREATE", account->accountID, NULL, amount, "success");
                            break;
                        }
                    }
                }
                else
                {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Account %s already exists\n", account->accountID);
                    logTransaction(shmPtr, "CREATE", account->accountID, NULL, 0, "failed");
                }
                isNewlyCreated = 0; 
            }
            else
            {
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s already exists\n", account->accountID);
                logTransaction(shmPtr, "CREATE", account->accountID, NULL, 0, "failed");
            }
        }
        else if (account->open)
        {
            if (strcmp(transactionType, "Deposit") == 0)
            {
                account->balance += amount;
                // Update shared memory balance for this account
                int accountIndex = findAccount(shmPtr, account->accountID);
                if (accountIndex != -1)
                {
                    shmPtr->accounts[accountIndex].balance = account->balance; 
                }
                else{
                    logTransaction(shmPtr, transactionType, account->accountID, "", amount, "failed");
                }
                snprintf(buffer, MAX_INPUT_LENGTH, "Deposited %d into account %s\n", amount, account->accountID);
                logTransaction(shmPtr, "DEPOSIT", account->accountID, NULL, amount, "success");
            }
            else if (strcmp(transactionType, "Withdraw") == 0)
            {
                if (account->balance >= amount)
                {
                    account->balance -= amount;
                    // Update shared memory balance for this account
                    int accountIndex = findAccount(shmPtr, account->accountID);
                    if (accountIndex != -1)
                    {
                        shmPtr->accounts[accountIndex].balance = account->balance; 
                    }
                    else{
                        logTransaction(shmPtr, transactionType, account->accountID, "", amount, "failed");
                    }
                    snprintf(buffer, MAX_INPUT_LENGTH, "Withdrew %d from account %s\n", amount, account->accountID);
                    logTransaction(shmPtr, "WITHDRAW", account->accountID, NULL, amount, "success");
                }
                else
                {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Insufficient funds in account %s\n", account->accountID);
                    logTransaction(shmPtr, "WITHDRAW", account->accountID, NULL, amount, "failed");
                }
            }
            else if (strcmp(transactionType, "Inquiry") == 0)
            {
                int accountIndex = findAccount(shmPtr, account->accountID);

                if (accountIndex != -1 && shmPtr->accounts[accountIndex].open)
                {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Account %s balance: %d\n",
                             account->accountID, shmPtr->accounts[accountIndex].balance);
                    logTransaction(shmPtr, "INQUIRY", account->accountID, "", (shmPtr->accounts[accountIndex].balance), "success");
                }
                else
                {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Account %s not found or closed\n", account->accountID);
                    logTransaction(shmPtr, "INQUIRY", account->accountID, "", (shmPtr->accounts[accountIndex].balance), "failed");
                }
            }
            else if (strcmp(transactionType, "Close") == 0)
            {
                account->open = 0;
                // Update shared memory account status
                int accountIndex = findAccount(shmPtr, account->accountID);
                if (accountIndex != -1)
                {
                    shmPtr->accounts[accountIndex].open = 0; 
                }
                else
                {
                    logTransaction(shmPtr, transactionType, account->accountID, "", amount, "failed");
                }
                snprintf(buffer, MAX_INPUT_LENGTH, "Account %s closed\n", account->accountID);
                logTransaction(shmPtr, "CLOSE", account->accountID, NULL, 0, "success");
            }
            else if (strcmp(transactionType, "Transfer") == 0)
            {
                int senderIndex = findAccount(shmPtr, account->accountID);
                int recipientIndex = findAccount(shmPtr, recipient);

                // Validate both accounts and sender's balance
                if (senderIndex != -1 && recipientIndex != -1 &&
                    shmPtr->accounts[senderIndex].open && shmPtr->accounts[recipientIndex].open &&
                    shmPtr->accounts[senderIndex].balance >= amount)
                {
                    // Update balances in shared memory
                    shmPtr->accounts[senderIndex].balance -= amount;
                    shmPtr->accounts[recipientIndex].balance += amount;

                    snprintf(buffer, MAX_INPUT_LENGTH, "Transferred %d from %s to %s\n", amount, account->accountID, recipient);
                    logTransaction(shmPtr, "TRANSFER", account->accountID, recipient, amount, "success");
                }
                else
                {
                    snprintf(buffer, MAX_INPUT_LENGTH, "Transfer failed from %s to %s\n", account->accountID, recipient);
                    logTransaction(shmPtr, "TRANSFER", account->accountID, recipient, amount, "failed");
                }
            }
            else
            {
                snprintf(buffer, MAX_INPUT_LENGTH, "Invalid operation\n");
            }
        }
        else
        {
            snprintf(buffer, MAX_INPUT_LENGTH, "Invalid operation or account is closed\n");
        }

        write(writeFd, buffer, strlen(buffer) + 1); 
        exitMonitorQueue(&queue, account->accountID); 
    }
}
//these take in shared memory pointers and expected number of users/accoutns and activate the queue and shared memory.
void initializeBankingSystem(int expectedNumUsers, int *shmID, SharedMemorySegment **shmPtr)
{
    initializeMonitorQueue(&queue, expectedNumUsers);
    initSharedMemory(shmID, shmPtr);
}
//destroys the targe shared memory Id and segment as well as the queue
void destroyBankingSystem(int shmID, SharedMemorySegment *shmPtr)
{
    destroyMonitorQueue(&queue);
    destroySharedMemory(shmID, shmPtr);
}
