// transactionHandler.h
// transactionHandler.h
#ifndef TRANSACTIONHANDLER_H
#define TRANSACTIONHANDLER_H

#include <sys/types.h>
#include "MonitorQueue.h"
#include "IPC_OS_Project.h"

#define MAX_ACCOUNTS 100
#define MAX_INPUT_LENGTH 100  

// Account structure
typedef struct {
    char accountID[20];
    int balance;
    int open;
    pid_t pid;
    int readFd;
    int writeFd;
} Account;

// Function declarations
int findAccount(Account accounts[], int numAccounts, char *accountID);
void handleTransaction(int readFd, int writeFd, Account *account, Account accounts[], int numAccounts, SharedMemorySegment *shmPtr);
void initializeBankingSystem(int expectedNumUsers, int *shmID, SharedMemorySegment **shmPtr);
void destroyBankingSystem(int shmID, SharedMemorySegment *shmPtr);

#endif // TRANSACTIONHANDLER_H
