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
using namespace std;

sem_t inUse;
sem_t notInUse;
bool isOccupied = false;
char *whoIsUsing;
pthread_mutex_t mutex;
const int PORT = 8081;
const int BUFFER_SIZE = 1000;

void *deviceAllocation(void *args);

int main(){
	//Mediator (Server)
	
	struct sockaddr_in serverAddress;	
	int addresslen = sizeof(serverAddress);
	int socketfd = socket(AF_INET, SOCK_STREAM, 0);
	int valueRead, opt = 1;
	char buffer[BUFFER_SIZE] = {0};

	//setsockopt
	/*
	if(setsockopt(socketfd,IPPROTO_TCP,SO_REUSEADDR,&opt,sizeof(opt)) != 0){ // returns -1 if failed
		printf("Failed to setsockopt socket");
		exit(1);
	}*/
	//binding mediator to socket
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	//inet_pton(AF_INET,"127.0.0.1",&serverAddress.sin_addr);
	bind(socketfd, (struct sockaddr *)&serverAddress, sizeof(serverAddress)); // returns -1 if failed
	
	//Listen for each process (Can be served in 2 different threads)
	if(listen(socketfd,10) < 0){
		printf("Listener failed");
		exit(1);
	}

	char *returnMsg = "Mediator - Permission Granted";
	struct sockaddr_in clientAddress1, clientAddress2;
	int newSocketClient1 , newSocketClient2;
	if( (newSocketClient1 = accept(socketfd,(struct sockaddr *)&clientAddress1,(socklen_t*)&addresslen)) < 0){
		printf("Error while accepting connection");
		exit(1);
	}	
	if( (newSocketClient2 = accept(socketfd,(struct sockaddr *)&clientAddress2,(socklen_t*)&addresslen)) < 0){
		printf("Error while accepting connection");
		exit(1);
	}


	printf("Established connections with both the clients \nNow creating two threads to serve requests");
	pthread_t threadId1 , threadId2;

	pthread_create(&threadId1,NULL,deviceAllocation,&newSocketClient1);
	pthread_create(&threadId2,NULL,deviceAllocation,&newSocketClient2);
	/*
	while(true){
		bzero(buffer,BUFFER_SIZE);
		read(newSocketClient1,buffer,BUFFER_SIZE);
		printf("Mediator - Msg received :  %s \n",buffer);
		//printf("Now writing msg to client");
		int a = write(newSocketClient1,returnMsg,BUFFER_SIZE);
		bzero(buffer,BUFFER_SIZE);
		//printf("No of bytes written = %d\n",a);
		printf("Mediator - Msg sent : %s\n",returnMsg);
	}
	*/

	close(socketfd);
	return 0;
}


void *deviceAllocation(void *args){
	int commfd = *(int *)args;
	char buffer[BUFFER_SIZE] = {0};
	char *requestFrom;
	char *requestType;
	while(true){
		read(commfd,buffer,BUFFER_SIZE);
		requestFrom = strtok(buffer,"#");
		requestType = strtok(NULL,"#");
		printf("Mediator - Request received : %s",buffer);
		//A = Access, R = Release
		if(requestType == "R"){
			pthread_mutex_lock(&mutex);
			isOccupied = false;
			whoIsUsing = NULL;
			char *reply = "Device Access Released";
			write(commfd,reply,BUFFER_SIZE);
			pthread_mutex_unlock(&mutex);
		}
		bool flag = true;	
		if(requestType == "A"){
			while(isOccupied){			
				if(flag){
					printf("Mediator - %s is using it now",whoIsUsing);
					flag = false;
				}
			}
			pthread_mutex_lock(&mutex);
			isOccupied = true;
			whoIsUsing = requestFrom;
			char *reply = "Device Access Granted";
			write(commfd,reply,BUFFER_SIZE);
			pthread_mutex_unlock(&mutex);
		}
	}
}
