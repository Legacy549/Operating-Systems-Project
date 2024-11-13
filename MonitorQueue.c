#include "MonitorQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

void initializeMonitorQueue(MonitorQueue *queue, int size) {
    queue->elements = malloc(size * sizeof(QueueElement));
    queue->maxSize = size;
    queue->front = 0;
    queue->rear = 0;
    pthread_mutex_init(&queue->queueLock, NULL);
}

void destroyMonitorQueue(MonitorQueue *queue) {
    free(queue->elements);
    pthread_mutex_destroy(&queue->queueLock);
}

void enterMonitorQueue(MonitorQueue *queue, char accountID[]) {
    pthread_mutex_lock(&queue->queueLock);
    printf("Entering monitor queue for account %s\n", accountID);
    displayQueueStatus(queue);
    pthread_mutex_unlock(&queue->queueLock);
}

void exitMonitorQueue(MonitorQueue *queue, char accountID[]) {
    pthread_mutex_lock(&queue->queueLock);
    printf("Exiting monitor queue for account %s\n", accountID);
    displayQueueStatus(queue);
    pthread_mutex_unlock(&queue->queueLock);
}

void enqueue(MonitorQueue *queue, QueueElement element) {
    pthread_mutex_lock(&queue->queueLock);
    queue->elements[queue->rear] = element;
    queue->rear = (queue->rear + 1) % queue->maxSize;
    pthread_mutex_unlock(&queue->queueLock);
}

QueueElement dequeue(MonitorQueue *queue) {
    pthread_mutex_lock(&queue->queueLock);
    QueueElement element = queue->elements[queue->front];
    queue->front = (queue->front + 1) % queue->maxSize;
    pthread_mutex_unlock(&queue->queueLock);
    return element;
}

void displayQueueStatus(const MonitorQueue *queue) {
    for (int i = queue->front; i != queue->rear; i = (i + 1) % queue->maxSize) {
        printf("[%s %d %d] ", queue->elements[i].accountID, queue->elements[i].transactionType, queue->elements[i].amount);
    }
    printf("\n");
}
