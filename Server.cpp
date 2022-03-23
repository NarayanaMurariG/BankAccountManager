#include <iostream>
#include <pthread.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <semaphore.h>
#include <strings.h>
#include <string.h>
#include <string>
#include <time.h>
#include <unistd.h>
#include <fstream>
#include <queue>
#include <sstream>
#include <unordered_map>
#include <chrono>
using namespace std;
using namespace std::chrono;

//All constants defined here
const int PORT = 8081;
const int BUFFER_SIZE = 1000;
const int TRANSACTION_POOL_SIZE = 5;
const int NAME_LENGTH = 100;
const int MAX_LISTEN_QUEUE_SIZE = 250;

//All function Declerations
void *transactionHandler(void *args);
void *serveClient(void *args);

/*
Creating an Account Information object structure containing
Account Number, Account Holder Name, Balance, Flag indicating if
account is being used for transaction and also each account has
its own mutex and conditional variable
*/
struct AccountInfo{
	int accountNumber;
	char accountHolderName[NAME_LENGTH];
	float balance;
	bool isInUse;
	pthread_mutex_t accountLock;
	pthread_cond_t accountCond;
};

//Global Variables
//Using an unordered_map to store the account information
unordered_map<int,AccountInfo> accountTable;
pthread_mutex_t clientMutex;
int clientCount = 0; //Keeps the count of number of clients which the server is serving at this moment
time_t currentTime;
int transactionCounter = 0; //Counter to count total number of transactions
long int avgTime = 0;


//Reads account details from Records.txt file and creates Account Objects
//For each line and puts into a unordered_map called accountTable
void loadAccountsFromFile(char* argv){
	string line;
	printf("Loading Records File from : %s \n",argv);
	// ifstream recordFile ("Records.txt");

	ifstream recordFile (argv);
	if(recordFile.is_open()){
		int accountNumber, balance;
		char buffer[NAME_LENGTH] = {0};
		while(getline(recordFile,line)){
			istringstream iss(line);

			/*
				istringstream splits the line extracts details like accountNumber,
				Account Holder Name, Inital Balance into the varibles declared
			*/
			if (!(iss >> accountNumber >> buffer >> balance)) { break; }
			struct AccountInfo accountInfo;
			accountInfo.accountNumber = accountNumber;
			strncpy(accountInfo.accountHolderName,buffer,sizeof(buffer));
			accountInfo.balance = balance;
			accountInfo.isInUse = false;

			//Initialising the mutexes and conditional variable for each account here
			if(pthread_mutex_init(&accountInfo.accountLock,NULL) == -1){
				cout << "Mutex Initalisation Failed" << '\n';
			}
			if(pthread_cond_init(&accountInfo.accountCond,NULL) == -1){
				cout << "Conditional Variable Initalisation Failed" << '\n';
			}

			accountTable.insert(make_pair(accountNumber,accountInfo));
			bzero(buffer,NAME_LENGTH);
		}
	}
	recordFile.close();

	/*
		unordered_map<int,AccountInfo>:: iterator itr;
		for(itr = accountTable.begin(); itr != accountTable.end() ; itr++){
		int accountNumber = itr->first;
	 	struct AccountInfo accountInfo= itr->second;
	 	printf("Key : %d \t",accountNumber);
	 	printf("Vale : AccountNumber : %d, Name : %s, balance : %f\n",accountInfo.accountNumber,accountInfo.accountHolderName,accountInfo.balance );
	} */
}

/*
	Once program reaches here means that, it has reached max client request for
	this run and it will destroy all the mutexes and conditionals variables
	 created for each account.
*/
void destroyAccountMutexes(){
	unordered_map<int,AccountInfo>:: iterator itr;
	for(itr = accountTable.begin(); itr != accountTable.end() ; itr++){
	if( pthread_mutex_destroy(&(itr->second.accountLock)) != 0){
		std::cout << "Mutex Destroy Failed" << '\n';
	}
	if( pthread_cond_destroy(&(itr->second.accountCond)) != 0){
		std::cout << "Cond Destroy Failed" << '\n';
	}

	}
}


int main(int argc,char* argv[]){

	if(argc == 1 ){
		printf("Please provide Records File Name  \n");
		exit(1);
	}

	printf("Server Coming up shortly....\n");


	//Creating accounts from Records.txt file
	loadAccountsFromFile(argv[1]);
	int whenToQuit = 0;
	bool dontQuit = true;
	if(argc > 2){
		whenToQuit = atoi(argv[2]);
		dontQuit = false;
		printf("Server will quit after %d connections\n",whenToQuit );
	}
	struct sockaddr_in serverAddress;
	int addresslen = sizeof(serverAddress);
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	int valueRead, opt = 1;
	char buffer[BUFFER_SIZE] = {0};


	//binding mediator to socket
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	//Binding socket to the port no defined in PORT
	if( bind(socketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == 0){ // returns -1 if failed
		printf("Binded Socket Successfully to the port number\n");
	} else{
		printf("Binding Socket Failed...\n Try After 10-15s \n");
		printf("Sometimes this happens when we stop server and start it immediately\n");
		destroyAccountMutexes();
		exit(0);
	}

	//Listen for new connections with queue size of 250
	if(listen(socketfd,MAX_LISTEN_QUEUE_SIZE) < 0){
		printf("Listener failed");
		destroyAccountMutexes();
		exit(1);
	}

	printf("Bringing up the listeners now.... Please wait before starting clients\n");
	sleep(5);
	printf("Start Clients Now\n");

	//initialising mutex to increment and decrement client counter
	pthread_mutex_init(&clientMutex,NULL);

	/*
		Keep Listening for new Clients and once client establishes connection
		serve that client in new thread which will run until client disconnects;
	*/
	printf("Ready to accept New connections \n");
	int i=0;
	pthread_t th[whenToQuit];
	while(dontQuit  || (i<whenToQuit) ){

		/* In this mode server runs indefinitely */
		if(dontQuit){
			int newClientSocket;
			pthread_t newThreadId;
			struct sockaddr_in newClientAddress;

			if( (newClientSocket = accept(socketfd,(struct sockaddr *)&newClientAddress,(socklen_t*)&addresslen)) < 0){
				printf("Error while accepting connection");
				exit(1);
			}
			else{
				currentTime = time(NULL);
				printf("%s -> Connected to a new Client and spwaning a thread in dontQuit mode\n",ctime(&currentTime));
				pthread_create(&newThreadId,NULL,&serveClient,&newClientSocket);
			}
		}else{
			/* This way is used if we want to test with multiple clients simultaneously
				or we want to limit the total no of clients server handles overall.
				If we pass 50 as argument, then code will exit this loop after it
				creates connections to 50 individual clients.
			*/
			int newClientSocket;
			struct sockaddr_in newClientAddress;

			if( (newClientSocket = accept(socketfd,(struct sockaddr *)&newClientAddress,(socklen_t*)&addresslen)) < 0){
				printf("Error while accepting connection");
				exit(1);
			}
			else{
				currentTime = time(NULL);
				printf("%s -> Connected to a new Client and spwaning a thread in quit Mode\n",ctime(&currentTime));
				pthread_create(&th[i],NULL,&serveClient,&newClientSocket);
				i++;
			}
		}

	}

	/*
	If the server is started in Quit mode (i.e) exits after serving specified no
	of clients then it has to wait till all the client threads are completely
	finished execution
	*/
	if(!dontQuit){
		printf("Waiting for threads to Join\n");
		int i=0;
		for (i = 0; i < whenToQuit; i++) {
        if (pthread_join(th[i], NULL) != 0) {
            perror("Failed to join the thread");
        }
    }
	}

	//Destroying the mutexes initialised during the intital setup
	destroyAccountMutexes();
	pthread_mutex_destroy(&clientMutex);

	//Closing the main socket descriptor
	close(socketfd);
	return 0;
}

void *serveClient(void *args){

	/*Using the mutex lock to increment the client counter
		Using mutex so that no two threads will simultaneously
		increase and decrease the current number of clients
	*/
	pthread_mutex_lock(&clientMutex);
	int clientNumber = ++clientCount;
	pthread_mutex_unlock(&clientMutex);
	int localTransactions = 0;
	printf("Connected to client : %d\n", clientNumber);

	int commfd = *(int *)args;
	char buffer[BUFFER_SIZE] = {0};
	int accountNumber;
	float amount;
	string transactionType;
	string accountNumberStr;
	string amountStr;
	string reply;
	while(true){
		read(commfd,buffer,BUFFER_SIZE);
		// printf("%s\n",buffer );
		if(strcmp(buffer,"EXIT") == 0){
			currentTime = time(NULL);
			printf("%s -> Closing Connection for Client Number : %d \n",ctime(&currentTime),clientNumber);
			reply = "Closing Connection";
			send(commfd,reply.c_str(),reply.size(),0);
			close(commfd);
			break;
		}else{
			//Parsing the values from received string
			auto begin = high_resolution_clock::now();
			// accountNumberStr = strtok(buffer,"\t");
			// transactionType = strtok(NULL,"\t");
			// amountStr = strtok(NULL,"\t");
			accountNumberStr = strtok(buffer," ");
			transactionType = strtok(NULL," ");
			amountStr = strtok(NULL," ");

			//Converting strings to integers and floats
			accountNumber = std::stoi(accountNumberStr);
			amount = std::stof(amountStr);
			currentTime = time(NULL);
			printf("%s -> %d - Data Received From Client %d\n",ctime(&currentTime),clientNumber,clientNumber);

			//Retreiving the account information from the accountTable
			unordered_map<int,AccountInfo>::iterator account = accountTable.find(accountNumber);

			if(account == accountTable.end()){ //If account isnt there, send the same to client
				currentTime = time(NULL);
				printf("%s -> %d - Account Number : %d , does not exist with this bank\n",ctime(&currentTime),clientNumber,accountNumber);
				string msg_str = "Account Does not exist with this bank";
				char *msg 	= &msg_str[0];
				send(commfd,msg,strlen(msg),0);
			}else{ //if account exists handle transaction
			transactionCounter++; //increment the transaction counters both local and global
			localTransactions++;
			string msg_str = "";
			float prevBalance = 0;
			//Lock the thread using account lock
			if(pthread_mutex_lock(&(account->second.accountLock)) != 0){
				cout << clientNumber << " Mutex Lock Failed "<< '\n';
			}

			/*If any prev thread is using the account object it has to go to
			conditional wait until that thread finishes using account object
			Initally account->second.isInUse = false; so first request will go through
			*/
			if(account->second.isInUse){
				currentTime = time(NULL);
				printf("%s -> %d - Waiting in Signal from other thread\n",ctime(&currentTime),clientNumber);

				if(transactionType.compare("d") == 0){
						printf("%s -> %d - Account Number : %d, Transaction Type : Deposit, Amount : %f\n", ctime(&currentTime),clientNumber,accountNumber,amount);
				}else{
						printf("%s -> %d - Account Number : %d, Transaction Type : Withdrawal, Amount : %f\n", ctime(&currentTime),clientNumber,accountNumber,amount);
				}
				int o = pthread_cond_wait(&(account->second.accountCond),&(account->second.accountLock));
				if(o != 0){
					cout << "Error on Wait" << '\n';
				}
				currentTime = time(NULL);
				printf("%s -> %d - Signal Recieved from other thread\n",ctime(&currentTime),clientNumber);
			}
		/*Now the thread which reaches here can change the status of account to
		Being in use and only when it releases other threads waiting in conditional
		wait will be able to access the account object*/
			account->second.isInUse = true;
			if(pthread_mutex_unlock(&(account->second.accountLock)) != 0){
				cout << clientNumber << " Mutex Unlock Failed " << '\n';
			}

			/*
				There might be a few scenarios that banking operations can take much
				longer times and during that time if any other request comes for same
				account it will be put in conditional wait.

				I have chosen this approach so as to not lock the account for the entire
				duration of the transaction period assuming that some transactions can
				take very long times and at the same time other requests for same account
				will be in wait and will wait for signal
			*/
			prevBalance = account->second.balance;
			if(transactionType.compare("d") == 0){ //deposit
				account->second.balance = account->second.balance + amount;
				currentTime = time(NULL);
				printf("%s -> %d - Transaction Success for AccountNumber : %d,Transaction Amount : %f, \nTransaction Type : Deposit , PrevBalance : %f, Final Balance : %f\n",ctime(&currentTime),clientNumber,accountNumber,amount,prevBalance,account->second.balance);
				msg_str = msg_str + "Transaction Success for AccountNumber : " +std::to_string(accountNumber)+ ", Transaction Type : Deposit ,PrevBalance : "+std::to_string(prevBalance)+", Final Balance : "+std::to_string(account->second.balance);
			}else{ //withdraw
				if(account->second.balance < amount){
					currentTime = time(NULL);
					printf("%s -> %d - Transaction Declined Due to Insufficient Balance. AccountNumber : %d,\nTransaction Amount : %f, AccountBalance : %f\n",ctime(&currentTime),clientNumber,accountNumber,amount,prevBalance);
					msg_str = msg_str + "Transaction Declined Due to Insufficient Balance. AccountNumber : "+std::to_string(accountNumber)+", Transaction Type : Withdrawal ,AccountBalance : "+std::to_string(prevBalance)+", Final Balance : "+std::to_string(account->second.balance);
				}else{
					currentTime = time(NULL);
					account->second.balance = account->second.balance - amount;
					printf("%s -> %d - Transaction Success for AccountNumber : %d,Transaction Amount : %f, \nTransaction Type : Withdrawal , PrevBalance : %f, Final Balance : %f\n",ctime(&currentTime),clientNumber,accountNumber,amount,prevBalance,account->second.balance);
					msg_str = msg_str + "Transaction Success for AccountNumber : "+std::to_string(accountNumber)+", Transaction Type : Withdrawal ,PrevBalance : "+std::to_string(prevBalance)+", Final Balance : "+std::to_string(account->second.balance);
				}
			}
			//comment below line for normal use
			// sleep(2); //using this sleep to test on conditionals and locks
			char *msg = &msg_str[0];
			auto end = high_resolution_clock::now();

			//Sending message to client
			send(commfd,msg,strlen(msg),0);
			auto duration = duration_cast<microseconds> (end -begin);
			avgTime = avgTime + duration.count();
			/*
			Locking the thread again to change isInUse to false.
			No other request in other thread can reach here as they will be put into
			wait queue as isInUse = true;
			*/
			if(pthread_mutex_lock(&(account->second.accountLock)) != 0){
				cout << clientNumber << " Mutex Lock Failed "<< '\n';
			}
			account->second.isInUse = false;
			if(pthread_mutex_unlock(&(account->second.accountLock)) != 0){
				cout << clientNumber << " Mutex Unlock Failed " << '\n';
			}

			//Sending signal to threads in conditional wait
			int o = pthread_cond_signal(&(account->second.accountCond));
			if(o != 0){
				std::cout << "Error on Wait" << '\n';
			}

		}
		bzero(buffer,BUFFER_SIZE);
		}
	}
		//Decrement client counter after locking the object and release the lock;
		pthread_mutex_lock(&clientMutex);
		clientCount--;
		pthread_mutex_unlock(&clientMutex);

		//Printing stats for this thread and avg time per tansaction till now
		printf("%d - Transactions Handled By this Thread : %d\n",clientNumber,localTransactions);
		printf("%d - Avg Time per transaction : %ld \n Total Transactions : %d \n",clientNumber,avgTime / transactionCounter, transactionCounter);

		return NULL;
}
