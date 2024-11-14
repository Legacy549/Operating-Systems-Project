// SyncronizationWithMonitors.c

//Author Name: Saah Sicarr
//Email: Saah.sicarr@Okstate.edu
//Date: 11/13/24
//Program Description: Combines Monitor Queue and Syncronization with Moinitors




#include "MonitorQueue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>

#define NUM_THREADS 2

typedef struct {
    char accountID[20];
    char name[50];
    int balance;
} Customer;

Customer customer;
sem_t semaphore;
MonitorQueue transactionQueue;

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

void *updateBalance(void *arg) {
    int amount;
    int choice;

    printf("Choose an operation:\n1. Deposit\n2. Withdraw\nEnter choice: ");
    scanf("%d", &choice);

    QueueElement transaction;
    strncpy(transaction.accountID, customer.accountID, sizeof(transaction.accountID));
    transaction.transactionType = choice;

    sem_wait(&semaphore);

    if (choice == 1) {
        printf("Enter amount to deposit: ");
        scanf("%d", &amount);
        transaction.amount = amount;
        customer.balance += amount;
        printf("Deposited %d. New balance: %d\n", amount, customer.balance);
    } else if (choice == 2) {
        printf("Enter amount to withdraw: ");
        scanf("%d", &amount);
        transaction.amount = amount;

        if (amount > customer.balance) {
            printf("Insufficient funds. Current balance: %d\n", customer.balance);
        } else {
            customer.balance -= amount;
            printf("Withdrew %d. New balance: %d\n", amount, customer.balance);
        }
    } else {
        printf("Invalid choice. Exiting transaction.\n");
    }

    enqueue(&transactionQueue, transaction);
    sem_post(&semaphore);

    pthread_exit(NULL);
}

void *readCustomer(void *arg) {
    sem_wait(&semaphore);
    printf("Current Balance: %d\n", customer.balance);
    sem_post(&semaphore);
    pthread_exit(NULL);
}

int main() {
    pthread_t threads[NUM_THREADS];

    sem_init(&semaphore, 0, 1);
    initializeMonitorQueue(&transactionQueue, 10);

    printf("Enter customer account ID: ");
    fgets(customer.accountID, sizeof(customer.accountID), stdin);
    customer.accountID[strcspn(customer.accountID, "\n")] = 0;

    printf("Enter customer name: ");
    fgets(customer.name, sizeof(customer.name), stdin);
    customer.name[strcspn(customer.name, "\n")] = 0;

    printf("Enter initial balance: ");
    scanf("%d", &customer.balance);

    pthread_create(&threads[0], NULL, updateBalance, NULL);
    pthread_create(&threads[1], NULL, readCustomer, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    printf("Final Transaction Queue Status:\n");
    displayQueueStatus(&transactionQueue);

    sem_destroy(&semaphore);
    destroyMonitorQueue(&transactionQueue);

    return EXIT_SUCCESS;
}
