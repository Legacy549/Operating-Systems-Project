// IPC_OS_Project.c

//Author Name: Jacob Mitchell
//Email: Jacob.a.mitchell@okstate.edu
//Date: 11/12/24
//Program Description: implements IPC

#include "IPC_OS_Project.h"
#include"transactionHandler.h"
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <errno.h>  // For ETIMEDOUT and error handling
#include "LockUtils.h"
#include <stdlib.h> // For exit()


// Initializes shared memory
void initSharedMemory(int *shmID, SharedMemorySegment **shmPtr) {
    *shmID = shmget(IPC_PRIVATE, sizeof(SharedMemorySegment), IPC_CREAT | 0666);
    if (*shmID == -1) {
        perror("Failed to create shared memory segment");
        exit(1);
    }

    *shmPtr = (SharedMemorySegment *)shmat(*shmID, NULL, 0);
    if (*shmPtr == (void *)-1) {
        perror("Failed to attach shared memory segment");
        exit(1);
    }

    (*shmPtr)->transactionCount = 0;
    (*shmPtr)->accountCount = 0;

    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT);

    // Initialize the mutex with attributes
    if (pthread_mutex_init(&(*shmPtr)->shmLock, &attr) != 0) {
        perror("Mutex initialization failed");
        exit(1);
    }

    pthread_mutexattr_destroy(&attr);
}


void logTransaction(SharedMemorySegment *shmPtr, const char *type, const char *accountID, 
                    const char *recipientID, int amount, const char *status) {
    // Check if the transaction log has space
    if (shmPtr->transactionCount >= MAX_TRANSACTIONS) {
        fprintf(stderr, "Transaction log is full. Cannot log more transactions.\n");
        return;
    }

    // Access the next log entry in the shared memory
    TransactionLog *logEntry = &shmPtr->transactions[shmPtr->transactionCount++];

    // Populate the log entry with transaction details
    strncpy(logEntry->transactionType, type, sizeof(logEntry->transactionType) - 1);
    strncpy(logEntry->accountID, accountID, sizeof(logEntry->accountID) - 1);
    if (recipientID) {
        strncpy(logEntry->recipientID, recipientID, sizeof(logEntry->recipientID) - 1);
    } else {
        logEntry->recipientID[0] = '\0'; // Clear recipientID if not provided
    }
    logEntry->amount = amount;
    strncpy(logEntry->status, status, sizeof(logEntry->status) - 1);

    // Add a timestamp
    time_t now = time(NULL);
    strftime(logEntry->timestamp, sizeof(logEntry->timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    // Debugging: Confirm the transaction was logged
    printf("Logged Transaction: %s, Account: %s, Amount: %d, Status: %s, Timestamp: %s\n",
           logEntry->transactionType, logEntry->accountID, logEntry->amount, logEntry->status, logEntry->timestamp);
}




// Creates a new account in shared memory
void createAccountInSharedMemory(const char *accountID, int initialBalance, SharedMemorySegment *shmPtr) {
    if (shmPtr->accountCount >= MAX_ACCOUNTS) {
        fprintf(stderr, "Error: Max account limit reached.\n");
        return;
    }

    // Check if the account already exists
    int accountIndex = findAccountInSharedMemory(shmPtr, accountID);
    if (accountIndex != -1) {
        printf("Account %s already exists.\n", accountID);
        return;
    }

    // Add the new account
    Account newAccount;
    strncpy(newAccount.accountID, accountID, sizeof(newAccount.accountID) - 1);
    newAccount.balance = initialBalance;
    newAccount.open = 1;

    // Insert the new account into shared memory
    shmPtr->accounts[shmPtr->accountCount] = newAccount;
    shmPtr->accountCount++;

    printf("Account %s created successfully with balance %d\n", accountID, initialBalance);
}

// Find account in shared memory
int findAccountInSharedMemory(SharedMemorySegment *shmPtr, const char *accountID) {
    for (int i = 0; i < shmPtr->accountCount; i++) {
        if (strcmp(shmPtr->accounts[i].accountID, accountID) == 0) {
            return i;  // Return the index of the account
        }
    }
    return -1;  // Account not found
}

// Destroys shared memory
void destroySharedMemory(int shmID, SharedMemorySegment *shmPtr) {
    pthread_mutex_destroy(&shmPtr->shmLock); // Destroy the mutex
    shmdt(shmPtr);  // Detach the shared memory segment
    shmctl(shmID, IPC_RMID, NULL);  // Remove the shared memory segment
}
