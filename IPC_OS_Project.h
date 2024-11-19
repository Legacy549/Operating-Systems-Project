// IPC_OS_Project.h

//Author Name: Jacob Mitchell
//Email: Jacob.a.mitchell@okstate.edu
//Date: 11/12/24
//Program Description: implements IPC header

#ifndef IPC_OS_PROJECT_H
#define IPC_OS_PROJECT_H

#include <pthread.h>
#include <time.h>

// Maximum number of accounts
#define MAX_ACCOUNTS 100
#define MAX_TRANSACTIONS 100

// Structure for Account
typedef struct {
    char accountID[20];
    int balance;
    int open; // 1 for open, 0 for closed
    pid_t pid; // Process ID for the account
    int readFd;  // File descriptor for reading
    int writeFd; // File descriptor for writing
} Account;

// Structure for Transaction Log
typedef struct {
    char transactionType[20];
    char accountID[20];
    char recipientID[20];
    int amount;
    char status[10];
    char timestamp[20];
} TransactionLog;

// Structure for Shared Memory Segment
typedef struct {
    Account accounts[MAX_ACCOUNTS];
    int accountCount;
    int transactionCount;
    TransactionLog transactions[1000]; // Assuming 1000 transactions max
    pthread_mutex_t shmLock;
} SharedMemorySegment;



// Function declarations
void initSharedMemory(int *shmID, SharedMemorySegment **shmPtr);
void destroySharedMemory(int shmID, SharedMemorySegment *shmPtr);
void logTransaction(SharedMemorySegment *shmPtr, const char *type, const char *accountID,
                    const char *recipient, int amount, const char *status);

void createAccountInSharedMemory(const char *accountID, int initialBalance, SharedMemorySegment *shmPtr);
int findAccountInSharedMemory(SharedMemorySegment *shmPtr, const char *accountID);

#endif // IPC_OS_PROJECT_H
