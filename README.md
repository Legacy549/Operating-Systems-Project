# How to run


## Compilation

Download the files to your prefered directory and navigate to your chosen directory. Make sure the each file and it's respective header file are present. 


Then compile the code using the "gcc" command
```bash
gcc driver.c MonitorQueue.c IPC_OS_Project.c transactionHandler.c -lpthread
```

## How to Run
Make sure compilation did not fail. There should be a file named a.out you can check with the following command:
```bash
ls
```
simply run a.out
```bash
./a.out your_input.txt
```
This was tested in csx2.cs.okstate.edu
