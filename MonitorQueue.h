//Author Name: Kobe Rowland
//Email: Kobe.rowland@okstate.edu
//Date: 11/2/2024
//Program Description: Initilization of Monitorqueue.c 

#ifndef MONITORQUEUE_H
#define MONITORQUEUE_H

#include <pthread.h>

// Define QueueElement and MonitorQueue structures
typedef struct {
    char accountID[20];
    int transactionType;
    int amount;
} QueueElement;

typedef struct {
    QueueElement *elements;
    int maxSize;
    int front;
    int rear;
    pthread_mutex_t queueLock;
} MonitorQueue;

// Function declarations
void initializeMonitorQueue(MonitorQueue *queue, int size);
void destroyMonitorQueue(MonitorQueue *queue);
void enterMonitorQueue(MonitorQueue *queue, char accountID[]);
void exitMonitorQueue(MonitorQueue *queue, char accountID[]);
void enqueue(MonitorQueue *queue, QueueElement element);
QueueElement dequeue(MonitorQueue *queue);
void displayQueueStatus(const MonitorQueue *queue);

#endif // MONITORQUEUE_H

