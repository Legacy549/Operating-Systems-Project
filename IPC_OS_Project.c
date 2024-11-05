#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <time.h>

#define SHM_SIZE sizeof(Transaction)  // Size of shared memory segment

// Transaction struct
typedef struct Transaction {
    char transactionType[20];
    char accountNumber[50];
    char secondaryAccountNumber[50];
    int amount;
    int status;  // 1 for success, 0 for failure
    time_t timeStamp;
} Transaction;

// Shared memory ID and mutex for synchronization
int shmid;
pthread_mutex_t mutex;

// Initialize shared memory
void initialize_shared_memory() {
    shmid = shmget(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget failed");
        exit(1);
    }
    printf("Shared memory initialized with ID: %d\n", shmid);
}

// Attach to shared memory and write transaction
void write_to_shared_memory(Transaction *transaction) {
    pthread_mutex_lock(&mutex);
    
    Transaction *shm_ptr = (Transaction *)shmat(shmid, NULL, 0);
    if (shm_ptr == (Transaction *)(-1)) {
        perror("shmat failed");
        exit(1);
    }

    *shm_ptr = *transaction;  // Copy transaction data to shared memory
    printf("Transaction written to shared memory: %s, %s, Amount: %d, Status: %d\n",
           transaction->transactionType, transaction->accountNumber, transaction->amount, transaction->status);

    shmdt(shm_ptr);  // Detach from shared memory
    pthread_mutex_unlock(&mutex);
}

// Attach to shared memory and read transaction
void read_from_shared_memory(Transaction *transaction) {
    pthread_mutex_lock(&mutex);
    
    Transaction *shm_ptr = (Transaction *)shmat(shmid, NULL, 0);
    if (shm_ptr == (Transaction *)(-1)) {
        perror("shmat failed");
        exit(1);
    }

    *transaction = *shm_ptr;  // Copy data from shared memory
    printf("Transaction read from shared memory: %s, %s, Amount: %d, Status: %d\n",
           transaction->transactionType, transaction->accountNumber, transaction->amount, transaction->status);

    shmdt(shm_ptr);  // Detach from shared memory
    pthread_mutex_unlock(&mutex);
}

// Parent process function for reading results
void parent_process() {
    Transaction transaction;
    read_from_shared_memory(&transaction);

    // Process the transaction or log results as needed
    printf("Parent read transaction: Type: %s, Account: %s, Amount: %d, Status: %d\n",
           transaction.transactionType, transaction.accountNumber, transaction.amount, transaction.status);
}

// Child process function for executing and writing a transaction
void child_process(const char *transactionType, const char *accountId, int amount) {
    Transaction transaction;
    strcpy(transaction.transactionType, transactionType);
    strcpy(transaction.accountNumber, accountId);
    transaction.amount = amount;
    transaction.status = 1;  // Assume success for simplicity
    transaction.timeStamp = time(NULL);

    write_to_shared_memory(&transaction);
}

int main() {
    pid_t pid;

    // Initialize shared memory and mutex
    initialize_shared_memory();
    pthread_mutex_init(&mutex, NULL);

    pid = fork();
    if (pid == -1) {
        perror("fork failed");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {
        // Child process: performs a sample transaction
        child_process("deposit", "Account1", 500);
    } else {
        // Parent process: waits and reads transaction results
        wait(NULL);  // Wait for child process to complete
        parent_process();
    }

    // Clean up
    pthread_mutex_destroy(&mutex);
    shmctl(shmid, IPC_RMID, NULL);  // Remove shared memory segment

    return 0;
}
