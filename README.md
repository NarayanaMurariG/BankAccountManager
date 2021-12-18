# Bank Account Manager

Centralized Multi-User Concurrent Bank Account Manager

## Server Execution Pattern
### To Run server indefinitely use below command
./Server Records.txt

## To stop Server after it has connected and served to x number of clients in lifetime
### use below command
./Server Records.txt x

Eg : ./Server Records.txt 100

## Client execution Pattern
### To Execute each Client individually and set interval gap between requests
./Client UniqueClientName TransactionFileName.txt requestTimeIntervalinMillis

Eg : ./Client Client1 Transactions.txt 1000

### To Execute Client and use timestamps from transactions.txt
Eg : ./Client Client1 Transactions.txt


### Make Commands
To clean the executables, use :
make clean
To compile the classes, use :
make compile
