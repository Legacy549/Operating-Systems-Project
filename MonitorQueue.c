//Author Name: Kobe Rowland
//Email: Kobe.rowland@okstate.edu
//Date: 11/2/2024
//Program Description: Implementation of queue. 

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


typedef struct Node{
    //all the information that nodes carry
    char *accountID;
    int type;
    int amount;
    int transactionID;
    struct Node* next;
}Node;

//array of all the different transaction types
char types[6][30] = {"CreateAccount", "Deposit", "Withdraw", "Transfer", "Balance", "CloseAccount"};
int transactionID = 0;

struct Node* head = NULL;
struct Node* tail = NULL;

//checks if linked list is empty and 
//returns boolean to use in other functions
bool isEmpty(){
    return head == NULL;
}

//prints the data of the first element
void peek(){
    if(!isEmpty()){
        printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Ammount:%d\n", head->transactionID, head->accountID, types[head->type], head->amount);
    }
    else{
        printf("Queue is empty\n");
    }
}

//prints entire queue
void printQueue(){
    if(isEmpty()){
        printf("Queue is empty");
        return;
    }

    printf("Current status of the Queue\n");
    printf("---------------------------------------------------------------------------\n");
    printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Ammount:%d\n", head->transactionID, head->accountID, types[head->type], head->amount);

    Node* currentPtr = head;

    //the head element is printed with the code above
    //then this while traverses the queue printing everything
    while(currentPtr->next != NULL){
        currentPtr = currentPtr->next;
        printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Ammount:%d\n", currentPtr->transactionID, currentPtr->accountID, types[currentPtr->type], currentPtr->amount);
    }

    printf("\n");
}

//adds transaction to the end of the queue
void enqueue (char accountID[], int type, int amount){
    Node* newNode = (Node*)malloc(sizeof(Node));
    if(!newNode){
        fprintf(stderr, "Memory allocation failed\n");
        return;
    }
    transactionID ++;
    newNode->transactionID = transactionID;
    newNode->accountID = accountID;
    newNode->type = type;
    newNode->amount = amount;
    newNode->next = NULL;
    
    if(isEmpty()){
        head = newNode;
        tail = newNode;
    }
    else{
        tail->next = newNode;
        tail = newNode;
    }
    printQueue();
}

//deletes transaction at front of queue
//probably will need to run the transaction 
void dequeue(){
    if(isEmpty()){
        printf("Queue is empty\n");
        return;
    }

    Node* temp = head;
    head = head->next;

    free(temp);
    if(head == NULL){
        tail = NULL;
    }
}



