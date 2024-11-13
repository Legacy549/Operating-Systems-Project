// IPC_OS_Project.c
#include "IPC_OS_Project.h"
#include <sys/shm.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// Initializes shared memory
void initSharedMemory(int *shmID, SharedMemorySegment **shmPtr) {
    *shmID = shmget(IPC_PRIVATE, sizeof(SharedMemorySegment), IPC_CREAT | 0666);
    *shmPtr = (SharedMemorySegment *)shmat(*shmID, NULL, 0);
    (*shmPtr)->transactionCount = 0;
    pthread_mutex_init(&(*shmPtr)->shmLock, NULL);
}

// Logs a transaction to shared memory
void logTransaction(SharedMemorySegment *shmPtr, const char *type, const char *accountID, 
                    const char *recipientID, int amount, const char *status) {
    pthread_mutex_lock(&shmPtr->shmLock);

    TransactionLog *logEntry = &shmPtr->transactions[shmPtr->transactionCount++];
    strncpy(logEntry->transactionType, type, sizeof(logEntry->transactionType) - 1);
    strncpy(logEntry->accountID, accountID, sizeof(logEntry->accountID) - 1);
    if (recipientID) strncpy(logEntry->recipientID, recipientID, sizeof(logEntry->recipientID) - 1);
    logEntry->amount = amount;
    strncpy(logEntry->status, status, sizeof(logEntry->status) - 1);

    // Add timestamp
    time_t now = time(NULL);
    strftime(logEntry->timestamp, sizeof(logEntry->timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));

    pthread_mutex_unlock(&shmPtr->shmLock);
}

// Cleans up shared memory
void destroySharedMemory(int shmID, SharedMemorySegment *shmPtr) {
    shmdt(shmPtr);
    shmctl(shmID, IPC_RMID, NULL);
}

