// transactionHandler.h

//Author Name: Karson Younger
//Email: karson.younger@okstate.edu
//Date: 11/17/24
//Program Description: Header for transactionHandler.c

#ifndef TRANSACTIONHANDLER_H
#define TRANSACTIONHANDLER_H

#include <sys/types.h>
#include "MonitorQueue.h"
#include "IPC_OS_Project.h"

#define MAX_ACCOUNTS 100
#define MAX_INPUT_LENGTH 100  

// Function declarations
void handleTransaction(int readFd, int writeFd, Account *account, SharedMemorySegment *shmPtr);
int findAccount(SharedMemorySegment *shmPtr, const char *accountID);
void initializeBankingSystem(int expectedNumUsers, int *shmID, SharedMemorySegment **shmPtr);
void destroyBankingSystem(int shmID, SharedMemorySegment *shmPtr);

#endif 
