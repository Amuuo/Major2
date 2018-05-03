#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <termio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

typedef int    SOCKET;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct sockaddr sockaddr;

#define MSG_BUFF_LENGTH 128
#define SERVER_NAME "SERVER"

typedef struct client {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
} client;

typedef struct server {
	SOCKET sockfd;
	sockaddr_in protocols;
	char* name;
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
} server;

void*  swapNames(void*);
void* sendMyName(void*);
void* getClientName(void*);
void  createSocket();
void  setupProtocols();
void  bindSocket();
void* listenAcceptSocket();
//void  communicate(int);
void* sending(void*);
void* receiving(void*);

//======================================
//          GLOBAL VARIABLES
//======================================

pthread_t    LISTEN_THREAD;
pthread_t    COMMUNICATE_THREAD;
pthread_t    RECEIVE_THREAD[2];
pthread_t    SENDING_THREAD[2];
pthread_t    SEND_NAME_THREAD;
pthread_t    GET_NAME_THREAD;
pthread_t    SWAP_THREAD;
hostent*     HOST;
client       CLIENT[2];
server       SERVER;
char*        HOSTNAME;
int          NUM_CONNECT_CLIENTS = 0;
int          SOCKADDR_IN_SIZE;

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	if (argc < 3) 
		printf("\nUsage: <hostname> <port>...");
	
	HOSTNAME = argv[1];
	SERVER.port = atoi(argv[2]);
	SERVER.name = "SERVER";
	createSocket();
	setupProtocols();
	bindSocket();
	pthread_create(&LISTEN_THREAD, NULL, &listenAcceptSocket, NULL);
	pthread_join(LISTEN_THREAD, NULL);
	//swapNames();
	//communicate();

	close(SERVER.sockfd);
	close(CLIENT[0].sockfd);
	close(CLIENT[1].sockfd);

	return 0;
}
//=================================================================
//=================================================================

void createSocket() {
	if ((SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		int err = errno;
		printf("\nCount not create socket : %d", err);
		exit(1);
	} 
	printf("\n>> Socket created.");

	return;
}
void setupProtocols() {
	memset(&HOST, 0, sizeof(HOST));
	HOST = gethostbyname(HOSTNAME);
	memset(&SERVER.protocols, 0, sizeof(SERVER.protocols));
	bcopy((char*)HOST->h_addr_list[0], (char*)&SERVER.protocols.sin_addr.s_addr, HOST->h_length);
	SERVER.protocols.sin_addr.s_addr = htonl(INADDR_ANY);
	SERVER.protocols.sin_family = AF_INET;
	SERVER.protocols.sin_port = htons(SERVER.port);
	printf("\n>> Protocols created.");

	return;
}
void* swapNames(void* sockNum) {
	pthread_create(&GET_NAME_THREAD, NULL, &getClientName, &sockNum);
	pthread_create(&SEND_NAME_THREAD, NULL, &sendMyName, &sockNum);
	pthread_join(GET_NAME_THREAD, NULL);
	pthread_join(SEND_NAME_THREAD, NULL);

	return NULL;
}

void* sendMyName(void* sockNum) {
	unsigned int id = *((int*)sockNum);
	unsigned int nameSize = sizeof(*SERVER[id].name);

	send(CLIENT[id].sockfd, SERVER_NAME, nameSize, 0);

	return NULL;
}
void* getClientName(void*sockNum) {
	unsigned int id = *((int*)sockNum);
	memset(CLIENT[id].name, 0, sizeof(CLIENT[id].name));
	recv(CLIENT[id].sockfd, CLIENT[id].name, sizeof(CLIENT[id].name), 0);
	CLIENT[id].name[strlen(CLIENT[id].name)] = '\0';

	return NULL;
}
void bindSocket() {
	if ((bind(SERVER.sockfd, (sockaddr*)&SERVER.protocols, sizeof(SERVER.protocols))) < 0) {
		int errorNumber = errno;
		printf("\nBind failed with error code: %d", errorNumber);
		exit(1);
	}
	printf("\n>> Bind Succeeded");

	return;
}
void* listenAcceptSocket() {
	unsigned int sockNum;
	while (NUM_CONNECT_CLIENTS < 3) {

		if ((listen(SERVER.sockfd, 2)) < 0) {
			int errorNumber = errno;
			printf("\n>> Listen failed, Error: %d", errorNumber);
			exit(1);
		}

		printf("\n>> Listening for incoming connections...");
		SOCKADDR_IN_SIZE = sizeof(struct sockaddr_in);

		if ((CLIENT[NUM_CONNECT_CLIENTS].sockfd = accept(SERVER.sockfd, 
			(sockaddr*)&CLIENT[NUM_CONNECT_CLIENTS].protocols, &SOCKADDR_IN_SIZE)) < 0) {
			int errorNumber = errno;
			printf("\nAccept failed with error code: %d", errorNumber);
			exit(1);
		} printf("\n>> Connection accepted.\n\n");

		++NUM_CONNECT_CLIENTS;

		// exit if more than 2 clients attempt to connect
		if (NUM_CONNECT_CLIENTS > 2) {
			printf("\nMore than 2 clients connected. Exiting");
			int i;
			for (i = 0; i < 3; ++i) {
				close(CLIENT[i].sockfd);
			}
			exit(2);
		}
		sockNum = NUM_CONNECT_CLIENTS - 1;
		pthread_create(&RECEIVE_THREAD[sockNum], NULL, &receiving, (void*)&sockNum);
		pthread_create(&SENDING_THREAD[sockNum], NULL, &sending, (void*)&sockNum);
		pthread_create(&SWAP_THREAD, NULL, &swapNames, (void*)&sockNum);
		//communicate(NUM_CONNECT_CLIENTS - 1);
	}

	return NULL;
}
/*void communicate(int sockNum) {

	pthread_join(RECEIVE_THREAD, NULL);
	pthread_join(SENDING_THREAD, NULL);

	return;
}*/
void* receiving(void* sockNum) {
	unsigned int bytesReceived;
	unsigned int id = *((int*)sockNum);
	
	while (1) {
		memset(SERVER.receive_msg_buff, 0, sizeof(SERVER.receive_msg_buff));
		bytesReceived = recv(CLIENT[id].sockfd, SERVER.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0);
		SERVER.receive_msg_buff[bytesReceived] = '\0';
		printf("\n%s: %s", CLIENT[id].name, SERVER.receive_msg_buff);
	}
	return NULL;
}
void* sending(void* sockNum) {
	char* message;
	unsigned int id = *((int*)sockNum);
	
	while (1) {
		printf("\n\n%s: ", SERVER_NAME);
		scanf("%s", message);
		printf("\n%s: %s", SERVER_NAME, message);
		send(CLIENT[id].sockfd, message, strlen(message), 0);
		if (message == '0') {
			--NUM_CONNECT_CLIENTS;
		}
		memset(message, 0, sizeof(*message));
	}
	
	return NULL;
}




