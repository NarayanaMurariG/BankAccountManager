#include<iostream>
#include<sys/socket.h>
#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
using namespace std;

const int BUFFER_SIZE = 1000;

int main(){
	
	char buffer[BUFFER_SIZE] = {0};
	struct sockaddr_in serverAddress;
	int socketfd = socket(AF_INET,SOCK_STREAM,0);
	int valueRead;

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(8081);
	inet_pton(AF_INET,"127.0.0.1",&serverAddress.sin_addr);
	connect(socketfd,(struct sockaddr *)&serverAddress,sizeof(serverAddress));
	int count = 0;
	printf("Client 1 - Process coming up...\n");
	while(true){		
		//write r to request access to device to mediator
		char *c = "C1#A";
		send(socketfd,c,strlen(c),0);
		printf("Client 1 - Sent message to mediator for device access \n");
		//read message from mediator
		valueRead = read(socketfd,buffer,BUFFER_SIZE);
		printf("Client 1 - Received from server %s \n",buffer);
		bzero(buffer,BUFFER_SIZE);
		//use device for sometime
		time_t t;
		srand((unsigned) time(&t));
		int time = 0;
		while(time == 0){
			time = rand() % 4;
		}
		printf("Client 1 - Sleeping for %d seconds\n",time);
		sleep(time);
		//write d to send message to mediator that use is over
		char *d = "C1#R";
		send(socketfd,d,strlen(d),0);
		printf("Client 1 - Sent message to mediator for device release \n");
		read(socketfd,buffer,BUFFER_SIZE);
		printf("Client 1 - Received from mediator :  %s \n",buffer);
		bzero(buffer,BUFFER_SIZE);
		count++;
		if(count == 4){
			close(socketfd);
			printf("Client shutting down");
			break;
		}
	}
	close(socketfd);

	return 0;
}
