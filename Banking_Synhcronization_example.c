#include <stdlib.h>
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

// Thread function for deposit or withdrawal
void *updateBalance(void *arg) {
    int amount;
    int choice;

    printf("Choose an operation:\n1. Deposit\n2. Withdraw\nEnter choice: ");
    scanf("%d", &choice);

    sem_wait(&semaphore);

    if (choice == 1) {
        printf("Enter amount to deposit: ");
        scanf("%d", &amount);
        customer.balance += amount;
        printf("Deposited %d. New balance: %d\n", amount, customer.balance);
    } else if (choice == 2) {
        printf("Enter amount to withdraw: ");
        scanf("%d", &amount);

        if (amount > customer.balance) {
            printf("Insufficient funds. Current balance: %d\n", customer.balance);
        } else {
            customer.balance -= amount;
            printf("Withdrew %d. New balance: %d\n", amount, customer.balance);
        }
    } else {
        printf("Invalid choice. Exiting transaction.\n");
    }

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

    // enter customer details
    printf("Enter customer account ID: ");
    fgets(customer.accountID, sizeof(customer.accountID), stdin);
    printf("Enter customer name: ");
    fgets(customer.name, sizeof(customer.name), stdin);

    printf("Enter initial balance: ");
    scanf("%d", &customer.balance);

    pthread_create(&threads[0], NULL, updateBalance, NULL);

    pthread_create(&threads[1], NULL, readCustomer, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);  
    }

    sem_destroy(&semaphore);

    return EXIT_SUCCESS;
}