//Author Name: Kobe Rowland
//Email: Kobe.rowland@okstate.edu
//Date: 11/2/2024
//Program Description: Implementation of queue. 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>  

#define QUEUE_SIZE 30  

// Shared node structure to represent a transaction
typedef struct Node {
    char accountID[50];
    char type[20];
    int amount;
    char recipient[50];
    int transactionID;
} Node;

// Shared queue structure that holds the queue data
typedef struct Queue {
    Node queue[QUEUE_SIZE];
    int front;
    int rear;
    // Semaphore for synchronization between processes
    sem_t lock;  
} SharedQueue;

// Shared memory segments for the queue and transactionID
SharedQueue *shared_queue;
int *transactionID;

// Function to create and attach the shared memory segments
int create_shared_memory() {
    // Generate a unique key for shared memory
    key_t key = ftok("/tmp", 'a');
    if (key == -1) {
        perror("ftok failed");
        return -1;
    }

    // Create shared memory for the queue
    int shm_id_queue = shmget(key, sizeof(SharedQueue), IPC_CREAT | 0666);
    if (shm_id_queue == -1) {
        perror("shmget failed for queue");
        return -1;
    }

    // Create shared memory for the global transactionID
    int shm_id_txn = shmget(key + 1, sizeof(int), IPC_CREAT | 0666);
    if (shm_id_txn == -1) {
        perror("shmget failed for transactionID");
        return -1;
    }

    // Attach shared memory segments
    shared_queue = (SharedQueue *)shmat(shm_id_queue, NULL, 0);
    if (shared_queue == (void *)-1) {
        perror("shmat failed for queue");
        return -1;
    }

    transactionID = (int *)shmat(shm_id_txn, NULL, 0);
    if (transactionID == (void *)-1) {
        perror("shmat failed for transactionID");
        return -1;
    }

    // Initialize the shared queue (front and rear pointers)
    shared_queue->front = 0;
    shared_queue->rear = 0;

    // Initialize semaphore for synchronization (using unnamed semaphore in shared memory)
    if (sem_init(&shared_queue->lock, 1, 1) == -1) {
        perror("sem_init failed");
        return -1;
    }

    // Initialize global transactionID
    *transactionID = 0;

    return 0;
}

// Function to check if the queue is empty
bool isEmpty() {
    return shared_queue->front == shared_queue->rear;
}


// Function to print the entire queue
void printQueue() {
    // Lock using semaphore
    sem_wait(&shared_queue->lock);  

    if (isEmpty()) {
        printf("Queue is empty\n");
    } else {
        printf("\nCurrent status of the Queue:\n");
        printf("---------------------------------------------------------------------------\n");

        // Print all transactions in the queue
        int i = shared_queue->front;
        while (i != shared_queue->rear) {
            printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Amount:%d, Recipient:%s\n", 
                    shared_queue->queue[i].transactionID, 
                    shared_queue->queue[i].accountID, 
                    shared_queue->queue[i].type, 
                    shared_queue->queue[i].amount, 
                    shared_queue->queue[i].recipient);
            i = (i + 1) % QUEUE_SIZE;
        }
    }
    // Unlock using semaphore
    sem_post(&shared_queue->lock);
}

// Function to dequeue a transaction from the front of the queue
void dequeue() {
    // Lock using semaphore
    sem_wait(&shared_queue->lock);  

    if (isEmpty()) {
        printf("Queue is empty\n");
    } else {
        // Remove transaction from the front of the queue
        shared_queue->front = (shared_queue->front + 1) % QUEUE_SIZE;
    }
    // Unlock using semaphore
    sem_post(&shared_queue->lock);  
}

// Function to enqueue a new transaction into the shared queue
void enqueue(char accountID[], char type[], int amount, char recipient[]) {
    sem_wait(&shared_queue->lock);  

    // Check if the queue is full
    if ((shared_queue->rear + 1) % QUEUE_SIZE == shared_queue->front) {
        printf("Queue is full, cannot enqueue transaction\n");
    } else {
        // Add the transaction to the queue
        (*transactionID)++;  // Increment global transactionID
        shared_queue->queue[shared_queue->rear].transactionID = *transactionID;
        strcpy(shared_queue->queue[shared_queue->rear].accountID, accountID);
        strcpy(shared_queue->queue[shared_queue->rear].type, type);
        shared_queue->queue[shared_queue->rear].amount = amount;
        strcpy(shared_queue->queue[shared_queue->rear].recipient, recipient);
        // Update the rear index
        shared_queue->rear = (shared_queue->rear + 1) % QUEUE_SIZE;
        
    }
    
    sem_post(&shared_queue->lock);  

    // Call printQueue after enqueue to show the current state of the queue
    printQueue();
}

// Function to clean up shared memory after use
void cleanup_shared_memory() {
    // Detach shared memory from the address space
    if (shmdt(shared_queue) == -1) {
        perror("shmdt failed for queue");
    }
    if (shmdt(transactionID) == -1) {
        perror("shmdt failed for transactionID");
    }
}

