#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


typedef struct Account
{
    char account_id[50];
    int balance;
    struct Account *next;
} Account;

typedef struct Resource
{
    char *name;
    pthread_mutex_t lock;
    int allocated;
} Resource;

typedef struct Process
{
    char *processId;
    char **requiredResources;
    int numResources;
} Process;

Resource *find_resource(Resource *resources, int resourceCount, const char *name)
{
    for (int i = 0; i < resourceCount; i++)
    {
        if (strcmp(resources[i].name, name) == 0)
        {
            return &resources[i];
        }
    }
    return NULL;
}

void create_account(const char *account_id, int initial_balance);
Account *find_account(const char *account_id);
void perform_transaction(const char *account_id, const char *transaction_type, int amount, const char *to_account_id);
int allocate_resources(Process process, Resource *resources, int resourceCount);
void release_resources(Process process, Resource *resources, int resourceCount);
void create_transaction(Process process, const char *account_id, const char *transaction_type, int amount, const char *to_account_id, Resource *resources, int resourceCount);
void manual_input(Resource *resources, int resourceCount);

Account *head = NULL;
pthread_mutex_t lock;

void create_account(const char *account_id, int initial_balance)
{
    Account *new_account = (Account *)malloc(sizeof(Account));
    strcpy(new_account->account_id, account_id);
    new_account->balance = initial_balance;
    new_account->next = head;
    head = new_account;
    printf("Account %s created with balance %d\n", account_id, initial_balance);
}

Account *find_account(const char *account_id)
{
    Account *current = head;
    while (current != NULL)
    {
        if (strcmp(current->account_id, account_id) == 0)
        {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void perform_transaction(const char *account_id, const char *transaction_type, int amount, const char *to_account_id)
{
    pthread_mutex_lock(&lock);
    Account *account = find_account(account_id);
    if (!account)
    {
        create_account(account_id, 0);
        account = find_account(account_id);
    }
    if (strcmp(transaction_type, "deposit") == 0)
    {
        account->balance += amount;
        printf("Deposited %d to account %s, new balance: %d\n", amount, account->account_id, account->balance);
    }
    else if (strcmp(transaction_type, "withdraw") == 0)
    {
        if (account->balance >= amount)
        {
            account->balance -= amount;
            printf("Withdrew %d from account %s, new balance: %d\n", amount, account->account_id, account->balance);
        }
        else
        {
            printf("Insufficient funds in account %s\n", account->account_id);
        }
    }
    else if (strcmp(transaction_type, "transfer") == 0 && to_account_id)
    {
        Account *to_account = find_account(to_account_id);
        if (!to_account)
        {
            create_account(to_account_id, 0);
            to_account = find_account(to_account_id);
        }
        if (account->balance >= amount)
        {
            account->balance -= amount;
            to_account->balance += amount;
            printf("Transferred %d from account %s to %s\n", amount, account->account_id, to_account->account_id);
        }
        else
        {
            printf("Insufficient funds in account %s\n", account->account_id);
        }
    }
    pthread_mutex_unlock(&lock);
}

int allocate_resources(Process process, Resource *resources, int resourceCount) {
    for (int i = 0; i < process.numResources; i++) {
        Resource *resource = find_resource(resources, resourceCount, process.requiredResources[i]);
        if (resource == NULL || resource->allocated) {
            for (int j = 0; j < i; j++) {
                Resource *prevResource = find_resource(resources, resourceCount, process.requiredResources[j]);
                pthread_mutex_unlock(&prevResource->lock);
                prevResource->allocated = 0;
            }
            return 0; // Allocation failed
        }
        pthread_mutex_lock(&resource->lock);
        resource->allocated = 1;
    }
    return 1; // Allocation succeeded
}

void release_resources(Process process, Resource *resources, int resourceCount) {
    for (int i = 0; i < process.numResources; i++) {
        Resource *resource = find_resource(resources, resourceCount, process.requiredResources[i]);
        pthread_mutex_unlock(&resource->lock);
        resource->allocated = 0;
    }
}

void create_transaction(Process process, const char *account_id, const char *transaction_type, int amount, const char *to_account_id, Resource *resources, int resourceCount)
{
    if (allocate_resources(process, resources, resourceCount))
    {
        perform_transaction(account_id, transaction_type, amount, to_account_id);
        release_resources(process, resources, resourceCount);
        printf("Transaction executed successfully.\n");
    }
    else
    {
        printf("Failed to allocate resources. Transaction aborted.\n");
    }
}

void deposit(Account *account, int amount)
{
    account->balance += amount;
    printf("Deposited %d to account %s, new balance: %d\n", amount, account->account_id, account->balance);
}

void withdraw(Account *account, int amount)
{
    if (account->balance >= amount)
    {
        account->balance -= amount;
        printf("Withdrew %d from account %s, new balance: %d\n", amount, account->account_id, account->balance);
    }
    else
    {
        printf("Insufficient funds in account %s\n", account->account_id);
    }
}

void transfer(Account *from_account, Account *to_account, int amount)
{
    if (from_account->balance >= amount)
    {
        from_account->balance -= amount;
        to_account->balance += amount;
        printf("Transferred %d from account %s to %s\n", amount, from_account->account_id, to_account->account_id);
    }
    else
    {
        printf("Insufficient funds in account %s\n", from_account->account_id);
    }
}

void file_input(const char* filename, Resource* resources, int resourceCount) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return;
    }

    int num_users;
    fscanf(file, "%d", &num_users);

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        char account_id[50], transaction_type[20], to_account_id[50];
        int amount;

        int fields_read = sscanf(line, "%s %s %d %s", account_id, transaction_type, &amount, to_account_id);

        if (strcmp(transaction_type, "Withdraw") == 0 && fields_read == 3) {
            Account* account = find_account(account_id);
            if (account) {
                Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                create_transaction(process, account_id, "withdraw", amount, NULL, resources, resourceCount);
            } else {
                printf("Account %s not found for withdrawal.\n", account_id);
            }
        } else if (strcmp(transaction_type, "Create") == 0 && fields_read == 3) {
            create_account(account_id, amount);
        } else if (strcmp(transaction_type, "Inquiry") == 0 && fields_read == 2) {
            Account* account = find_account(account_id);
            if (account) {
                printf("Account %s balance: %d\n", account->account_id, account->balance);
            } else {
                printf("Account %s not found for inquiry.\n", account_id);
            }
        } else if (strcmp(transaction_type, "Deposit") == 0 && fields_read == 3) {
            Account* account = find_account(account_id);
            if (account) {
                Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                create_transaction(process, account_id, "deposit", amount, NULL, resources, resourceCount);
            } else {
                printf("Account %s not found for deposit.\n", account_id);
            }
        } else if (strcmp(transaction_type, "Transfer") == 0 && fields_read == 4) {
            Account* from_account = find_account(account_id);
            Account* to_account = find_account(to_account_id);
            if (from_account && to_account) {
                Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                create_transaction(process, account_id, "transfer", amount, to_account_id, resources, resourceCount);
            } else {
                printf("Account(s) not found for transfer.\n");
            }
        } else if (strcmp(transaction_type, "Close") == 0 && fields_read == 2) {
            Account* account = find_account(account_id);
            if (account) {
                pthread_mutex_lock(&lock);
                if (head == account) {
                    head = account->next;
                } else {
                    Account* prev = head;
                    while (prev->next && prev->next != account) {
                        prev = prev->next;
                    }
                    if (prev->next == account) {
                        prev->next = account->next;
                    }
                }
                free(account);
                pthread_mutex_unlock(&lock);
                printf("Account %s closed.\n", account_id);
            } else {
                printf("Account %s not found for closure.\n", account_id);
            }
        } else {
            printf("Invalid transaction or format: %s", line);
        }
    }

    fclose(file);
}


void manual_input(Resource *resources, int resourceCount) {void manual_input(Resource *resources, int resourceCount) {
    char account_id[50], to_account_id[50];
    int amount, choice;

    while (1) {
        printf("Select transaction type:\n");
        printf("1. Create Account\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Transfer\n");
        printf("5. View Account Balance\n");
        printf("6. Back to Main Menu\n");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter account ID: ");
                scanf("%s", account_id);
                printf("Enter initial balance: ");
                scanf("%d", &amount);
                create_account(account_id, amount);
                break;
            case 2:
                printf("Enter account ID: ");
                scanf("%s", account_id);
                Account* account = find_account(account_id);
                if (account) {
                    printf("Enter amount: ");
                    scanf("%d", &amount);
                    Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                    create_transaction(process, account_id, "deposit", amount, NULL, resources, resourceCount);
                } else {
                    printf("Account not found.\n");
                }
                break;
            case 3:
                printf("Enter account ID: ");
                scanf("%s", account_id);
                account = find_account(account_id);
                if (account) {
                    printf("Enter amount: ");
                    scanf("%d", &amount);
                    Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                    create_transaction(process, account_id, "withdraw", amount, NULL, resources, resourceCount);
                } else {
                    printf("Account not found.\n");
                }
                break;
            case 4:
                printf("Enter your account ID: ");
                scanf("%s", account_id);
                Account* from_account = find_account(account_id);
                if (from_account) {
                    printf("Enter recipient account ID: ");
                    scanf("%s", to_account_id);
                    Account* to_account = find_account(to_account_id);
                    if (to_account) {
                        printf("Enter amount: ");
                        scanf("%d", &amount);
                        Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                        create_transaction(process, account_id, "transfer", amount, to_account_id, resources, resourceCount);
                    } else {
                        printf("Recipient account not found.\n");
                    }
                } else {
                    printf("Your account not found.\n");
                }
                break;
            case 5:
                printf("Enter account ID to view balance: ");
                scanf("%s", account_id);
                account = find_account(account_id);
                if (account) {
                    printf("Account %s balance: %d\n", account->account_id, account->balance);
                } else {
                    printf("Account not found.\n");
                }
                break;
            case 6:
                return; // Return to main menu
            default:
                printf("Invalid choice!\n");
                break;
        }
        printf("Press any key to continue...\n");
        getchar(); // consume newline
        getchar(); // wait for user to press a key
    }
}

    char account_id[50], to_account_id[50];
    int amount, choice;

    while (1) {
        printf("Select transaction type:\n");
        printf("1. Create Account\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Transfer\n");
        printf("5. View Account Balance\n");
        printf("6. Back to Main Menu\n");
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                printf("Enter account ID: ");
                scanf("%s", account_id);
                printf("Enter initial balance: ");
                scanf("%d", &amount);
                create_account(account_id, amount);
                break;
            case 2:
                printf("Enter account ID: ");
                scanf("%s", account_id);
                Account* account = find_account(account_id);
                if (account) {
                    printf("Enter amount: ");
                    scanf("%d", &amount);
                    Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                    create_transaction(process, account_id, "deposit", amount, NULL, resources, resourceCount);
                } else {
                    printf("Account not found.\n");
                }
                break;
            case 3:
                printf("Enter account ID: ");
                scanf("%s", account_id);
                account = find_account(account_id);
                if (account) {
                    printf("Enter amount: ");
                    scanf("%d", &amount);
                    Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                    create_transaction(process, account_id, "withdraw", amount, NULL, resources, resourceCount);
                } else {
                    printf("Account not found.\n");
                }
                break;
            case 4:
                printf("Enter your account ID: ");
                scanf("%s", account_id);
                Account* from_account = find_account(account_id);
                if (from_account) {
                    printf("Enter recipient account ID: ");
                    scanf("%s", to_account_id);
                    Account* to_account = find_account(to_account_id);
                    if (to_account) {
                        printf("Enter amount: ");
                        scanf("%d", &amount);
                        Process process = {"TransactionProcess", (char*[]){"Resource1", "Resource2"}, 2};
                        create_transaction(process, account_id, "transfer", amount, to_account_id, resources, resourceCount);
                    } else {
                        printf("Recipient account not found.\n");
                    }
                } else {
                    printf("Your account not found.\n");
                }
                break;
            case 5:
                printf("Enter account ID to view balance: ");
                scanf("%s", account_id);
                account = find_account(account_id);
                if (account) {
                    printf("Account %s balance: %d\n", account->account_id, account->balance);
                } else {
                    printf("Account not found.\n");
                }
                break;
            case 6:
                return; // Return to main menu
            default:
                printf("Invalid choice!\n");
                break;
        }
        printf("Press any key to continue...\n");
        getchar(); // consume newline
        getchar(); // wait for user to press a key
    }
}


int main() {
    pthread_mutex_init(&lock, NULL);

    // Initialize resources
    Resource resources[] = {
        {"Resource1", PTHREAD_MUTEX_INITIALIZER, 0},
        {"Resource2", PTHREAD_MUTEX_INITIALIZER, 0},
    };
    int resourceCount = sizeof(resources) / sizeof(resources[0]);

    while (1) {
        printf("Select input method:\n");
        printf("1. Manual Input\n");
        printf("2. File Input\n");
        printf("3. Exit\n");

        int choice;
        scanf("%d", &choice);

        switch (choice) {
            case 1:
                manual_input(resources, resourceCount);
                break;
            case 2: {
                char filename[100];
                printf("Enter filename: ");
                scanf("%s", filename);
                file_input(filename, resources, resourceCount);
                break;
            }
            case 3:
                printf("Exiting program.\n");
                pthread_mutex_destroy(&lock);
                return 0;
            default:
                printf("Invalid choice!\n");
                break;
        }
        printf("Press any key to continue...\n");
        getchar(); // consume newline
        getchar(); // wait for user to press a key
    }

    pthread_mutex_destroy(&lock);
    return 0;
}



