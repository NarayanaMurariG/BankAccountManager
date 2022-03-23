#include<iostream>
#include<sys/socket.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<fstream>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <string>
#include <sstream>
#include <fstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <netinet/in.h>
#include <chrono>
using namespace std;
using namespace std::chrono;
const int BUFFER_SIZE = 200;
time_t currentTime;
int clientNumber = 0;
int sleepTime = 0;
bool useDefaultSleep = false;

void* startThread(void* args) {
	int myNumber = ++clientNumber;
	printf("%d -> Connecting to Bank Server from Client-%d\n",myNumber,myNumber);
	printf("%d -> Loading Transaction details from Transactions.txt \n",myNumber);
	int socketfd;
	struct sockaddr_in serverAddress;
	socketfd = socket(AF_INET,SOCK_STREAM,0);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8081);
	inet_pton(AF_INET,"127.0.0.1",&serverAddress.sin_addr);
	connect(socketfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress));

	string line;
	ifstream transactionFile ("Transactions.txt");
	char buffer[BUFFER_SIZE] = {0};
	if(transactionFile.is_open()){
		int accountNumber, amount;
		char *transactionType;
		int waitTime;
		while(getline(transactionFile,line)){
			// string token = line.substr(line.find("\t")+1,line.size());
			string token = line.substr(line.find(" ")+1,line.size());
			istringstream iss(line);
			if (!(iss >> waitTime >> accountNumber >> buffer >> amount)) { break; }
			if(useDefaultSleep){
					usleep(sleepTime * 1000); //sleepTime will be taken in MilliSecs
			}else{
					sleep(waitTime);
			}
			currentTime = time(NULL);
			printf("%s %d -> Sending request to server -> %d\t%s\t%d\n",ctime(&currentTime),myNumber,accountNumber,buffer,amount);
			bzero(buffer,BUFFER_SIZE);
			send(socketfd,token.c_str(),token.size(),0);
			read(socketfd,buffer,BUFFER_SIZE);
			currentTime = time(NULL);
			printf("%s %d -> %s \n",ctime(&currentTime),myNumber,buffer);
			//sleep(waitTime);
			bzero(buffer,BUFFER_SIZE);
		}
	}
	transactionFile.close();

	//Sending final msg to close this connection and terminate the thread;
	string token = "EXIT";
	printf("%d -> Sent msg to server to close this connection\n",myNumber);
	send(socketfd,token.c_str(),token.size(),0);
	bzero(buffer,BUFFER_SIZE);
	read(socketfd,buffer,BUFFER_SIZE);
	cout << buffer << endl;
	close(socketfd);
	return NULL;
}

int main(int argc,char* argv[]){
	int noOfClients = 0;
	if(argc > 1){
		noOfClients = atoi(argv[1]);
	}else{
		printf("Please enter number of Clients \n");
		exit(0);
	}
	if(argc > 2){
		sleepTime = atoi(argv[2]);
		printf("Taking SleepTime Input from User : %d\n",sleepTime);
		useDefaultSleep = true;
	}else{
		printf("TimeStamps from Transactions.txt are being used\n");
	}
	//connecting to Bank Server using socket
	pthread_t th[noOfClients];
	int i;
	for (i = 0; i < noOfClients; i++) {
			if (pthread_create(&th[i], NULL, &startThread, NULL) != 0) {
					perror("Failed to create the thread");
			}
	}

	for (i = 0; i < noOfClients; i++) {
			if (pthread_join(th[i], NULL) != 0) {
					perror("Failed to join the thread");
			}
	}
	return 0;
}
