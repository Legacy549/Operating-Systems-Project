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
    char *type;
    int amount;
    char *recipient;
    int transactionID;
    struct Node* next;
}Node;

//array of all the different transaction types
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
        printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Amount:%d, Recipient: %s\n", 
        head->transactionID, head->accountID, head->type, head->amount, head->recipient);
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
    printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Amount:%d, Recipient: %s\n", 
    head->transactionID, head->accountID, head->type, head->amount, head->recipient);

    Node* currentPtr = head;

    //the head element is printed with the code above
    //then this while traverses the queue printing everything
    while(currentPtr->next != NULL){
        currentPtr = currentPtr->next;
        printf("Transaction ID:%d, Account ID:%s, Transaction type:%s, Amount:%d, Recipient: %s\n", 
        currentPtr->transactionID, currentPtr->accountID, currentPtr->type, currentPtr->amount, currentPtr->recipient);
    }

    printf("\n");
}

//adds transaction to the end of the queue
void enqueue (char accountID[], char type[], int amount, char recipient[]){
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
    newNode->recipient = recipient;
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

//used to possibly get infomation from queue
struct Node headNode(){
    Node temp = {head->accountID, head->amount,head->type, head->recipient};
    return temp;
}

//deletes transaction at front of queue
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



