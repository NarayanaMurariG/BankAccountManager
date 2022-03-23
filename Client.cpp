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
using namespace std;

const int BUFFER_SIZE = 200;
time_t currentTime;
int clientNumber = 0;
int sleepTime = 0;
bool useDefaultSleep = false;

int main(int argc,char* argv[]){

	if(argc == 1 ){
		printf("Please provide client Name, transactionfile name, sleepTime (Optional) \n");
		exit(1);
	}

	if(argv[2] == NULL){
		printf("Please provide Transaction File Name and use below Information\n");
		printf("Please provide client Name, transactionfile name, sleepTime (Optional) \n");
		exit(1);
	}

	if(argc > 3){ //It means user has given default sleepTime in MilliSecs
		sleepTime = atoi(argv[3]);
		printf("Taking SleepTime Input from User : %d\n",sleepTime);
		useDefaultSleep = true;
	}else{
		printf("TimeStamps from Transactions.txt are being used\n");
	}

	//connecting to Bank Server using socket
	printf("Connecting to Bank Server from Client : %s\n",argv[1]);
	printf("Loading Transaction details from %s \n",argv[2]);
	int socketfd;
	struct sockaddr_in serverAddress;
	socketfd = socket(AF_INET,SOCK_STREAM,0);

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8081);
	inet_pton(AF_INET,"127.0.0.1",&serverAddress.sin_addr);
	connect(socketfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress));

	//Reading line by line from the File Name provided by User
	string line;
	ifstream transactionFile (argv[2]);
	char buffer[BUFFER_SIZE] = {0};
	if(transactionFile.is_open()){
		int accountNumber, amount;
		char *transactionType;
		int waitTime;
		while(getline(transactionFile,line)){
			string token = line.substr(line.find(" ")+1,line.size());

			/*
				istringstream splits the line extracts details like accountNumber,
				Account Holder Name, Inital Balance into the varibles declared
			*/
			istringstream iss(line);
			if (!(iss >> waitTime >> accountNumber >> buffer >> amount)) { break; }

			if(useDefaultSleep){
					usleep(sleepTime * 1000); //sleepTime will be taken in MilliSecs
			}else{
					sleep(waitTime);
			}

			currentTime = time(NULL);
			printf("%s -> Sending request to server -> %d\t%s\t%d\n",ctime(&currentTime),accountNumber,buffer,amount);
			bzero(buffer,BUFFER_SIZE);

			//Sending line to server after removing the timestamp
			send(socketfd,token.c_str(),token.size(),0);

			//Reading reply from server
			read(socketfd,buffer,BUFFER_SIZE);
			currentTime = time(NULL);
			printf("%s -> %s \n",ctime(&currentTime),buffer);
			bzero(buffer,BUFFER_SIZE);
		}
	}
	transactionFile.close();

	//Sending final msg to close this connection and terminate the thread;
	string token = "EXIT";
	printf("Sent msg to server to close this connection\n");
	send(socketfd,token.c_str(),token.size(),0);
	bzero(buffer,BUFFER_SIZE);
	read(socketfd,buffer,BUFFER_SIZE);
	cout << buffer << endl;
	close(socketfd);

	return 0;
}
