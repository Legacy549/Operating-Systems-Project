// IPC_OS_Project.h

//Author Name: Jacob Mitchell
//Email: Jacob.a.mitchell@okstate.edu
//Date: 11/12/24
//Program Description: implements IPC
#ifndef IPC_OS_PROJECT_H
#define IPC_OS_PROJECT_H

#include <pthread.h>

typedef struct {
    // Define any necessary shared data structures here
    // For example, transaction data types
    char transactionType[20];
    char accountID[20];
    char recipientID[20];
    int amount;
    char status[10];
    char timestamp[30];
} TransactionLog;

typedef struct {
    TransactionLog transactions[1000];  // Max transactions
    int transactionCount;
    pthread_mutex_t shmLock;
} SharedMemorySegment;

// Function prototypes for shared memory operations
void initSharedMemory(int *shmID, SharedMemorySegment **shmPtr);
void logTransaction(SharedMemorySegment *shmPtr, const char *type, const char *accountID, 
                    const char *recipientID, int amount, const char *status);
void destroySharedMemory(int shmID, SharedMemorySegment *shmPtr);

#endif // IPC_OS_PROJECT_H

