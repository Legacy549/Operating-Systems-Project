//MonitorQueue.h

//Author Name: Kobe Rowland
//Email: Kobe.rowland@okstate.edu
//Date: 11/2/2024
//Program Description: Initilization of Monitorqueue.c 

#ifndef MONITOR_QUEUE_H
#define MONITOR_QUEUE_H

#include <pthread.h>

#define MAX_QUEUE_SIZE 100
#define MAX_ACCOUNT_ID_LEN 20

// Structure to represent a queue element
typedef struct {
    char accountID[MAX_ACCOUNT_ID_LEN];  // Account ID for the transaction
    int transactionType;  // Transaction type (deposit/withdrawal, etc.)
    int amount;  // Transaction amount
} QueueElement;

// Structure to represent a monitor queue
typedef struct {
    QueueElement *elements;
    int front;
    int rear;
    int maxSize;
    pthread_mutex_t queueLock;  // Mutex for thread synchronization
} MonitorQueue;

// Function prototypes for queue management
void initializeMonitorQueue(MonitorQueue *queue, int size);
void destroyMonitorQueue(MonitorQueue *queue);
void enterMonitorQueue(MonitorQueue *queue, char accountID[]);
void exitMonitorQueue(MonitorQueue *queue, char accountID[]);
void enqueue(MonitorQueue *queue, QueueElement element);
QueueElement dequeue(MonitorQueue *queue);
void displayQueueStatus(const MonitorQueue *queue);

#endif // MONITOR_QUEUE_H
