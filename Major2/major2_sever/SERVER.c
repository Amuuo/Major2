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

#define MSG_BUFF_LENGTH 32
#define SERVER_NAME "SERVER"

typedef struct client {
	SOCKET sockfd;
	sockaddr_in client_settings;
	char name[20];
} client;

void  swapNames(int);
void* sendMyName(void*);
void* getClientName(void*);
void  createSocket();
void  setupProtocols();
void  bindSocket();
void* listenAcceptSocket();
void  communicate(int);
void* sending(void*);
void* receiving(void*);

//======================================
//          GLOBAL VARIABLES
//======================================

sockaddr_in  SEVER_SETTINGS;
pthread_t    LISTEN_THREAD;
pthread_t    COMMUNICATE_THREAD;
pthread_t    RECEIVE_THREAD;
pthread_t    SENDING_THREAD;
pthread_t    SEND_NAME;
pthread_t    GET_NAME;
hostent*     HOST;
SOCKET       LOCAL_BIND_SOCKET;
client       CLIENT[2];
char         SEND_MESSAGE_BUFF[MSG_BUFF_LENGTH];
char         RECEIVE_MESSAGE_BUFF[MSG_BUFF_LENGTH];
char*        HOSTNAME;
int          LOCAL_PORT;
int          NUM_CONNECT_CLIENTS = 0;
int          SOCKADDR_IN_SIZE;

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	if (argc < 3) 
		printf("\nUsage: <hostname> <port>...");
	
	HOSTNAME = argv[1];
	LOCAL_PORT = atoi(argv[2]);
	createSocket();
	setupProtocols();
	bindSocket();
	pthread_create(&LISTEN_THREAD, NULL, &listenAcceptSocket, NULL);
	pthread_join(LISTEN_THREAD, NULL);
	//swapNames();
	//communicate();

	close(LOCAL_BIND_SOCKET);
	close(CLIENT[0].sockfd);
	close(CLIENT[1].sockfd);

	return 0;
}
//=================================================================
//=================================================================

void createSocket() {
	if ((LOCAL_BIND_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
	memset(&SEVER_SETTINGS, 0, sizeof(SEVER_SETTINGS));
	bcopy((char*)HOST->h_addr_list[0], (char*)&SEVER_SETTINGS.sin_addr.s_addr, HOST->h_length);
	SEVER_SETTINGS.sin_addr.s_addr = htonl(INADDR_ANY);
	SEVER_SETTINGS.sin_family = AF_INET;
	SEVER_SETTINGS.sin_port = htons(LOCAL_PORT);
	printf("\n>> Protocols created.");

	return;
}
void swapNames(int sockNum) {
	pthread_create(&GET_NAME, NULL, &getClientName, (void*)&sockNum);
	pthread_create(&SEND_NAME, NULL, &sendMyName, (void*)&sockNum);
	pthread_join(GET_NAME, NULL);
	pthread_join(SEND_NAME, NULL);

	return;
}

void* sendMyName(void* sockNum) {
	unsigned int nameSize = sizeof(CLIENT[*((int*)sockNum)].name);
	send(CLIENT[*((int*)sockNum)].sockfd, SERVER_NAME, nameSize, 0);

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
	if ((bind(LOCAL_BIND_SOCKET, (sockaddr*)&SEVER_SETTINGS, sizeof(SEVER_SETTINGS))) < 0) {
		int errorNumber = errno;
		printf("\nBind failed with error code: %d", errorNumber);
		exit(1);
	}
	printf("\n>> Bind Succeeded");

	return;
}
void* listenAcceptSocket() {
	while (NUM_CONNECT_CLIENTS < 2) {

		if ((listen(LOCAL_BIND_SOCKET, 2)) < 0) {
			int errorNumber = errno;
			printf("\n>> Listen failed, Error: %d", errorNumber);
			exit(1);
		}

		printf("\n>> Listening for incoming connections...");
		SOCKADDR_IN_SIZE = sizeof(struct sockaddr_in);

		if ((CLIENT[NUM_CONNECT_CLIENTS].sockfd = accept(LOCAL_BIND_SOCKET, 
			(sockaddr*)&CLIENT[NUM_CONNECT_CLIENTS].client_settings, &SOCKADDR_IN_SIZE)) < 0) {
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
		swapNames(NUM_CONNECT_CLIENTS - 1);
		communicate(NUM_CONNECT_CLIENTS - 1);
	}

	return NULL;
}
void communicate(int sockNum) {
	pthread_create(&RECEIVE_THREAD, NULL, &receiving, (void*)&sockNum);
	pthread_create(&SENDING_THREAD, NULL, &sending, (void*)&sockNum);
	pthread_join(RECEIVE_THREAD, NULL);
	pthread_join(SENDING_THREAD, NULL);

	return;
}
void* receiving(void* sockNum) {
	unsigned int bytesReceived;
	unsigned int id = *((int*)sockNum);
	memset(RECEIVE_MESSAGE_BUFF, 0, sizeof(RECEIVE_MESSAGE_BUFF));
	bytesReceived = recv(CLIENT[id].sockfd, RECEIVE_MESSAGE_BUFF, MSG_BUFF_LENGTH - 1, 0);
	RECEIVE_MESSAGE_BUFF[bytesReceived] = '\0';
	printf("\n\t%s: %s", CLIENT[id].name, RECEIVE_MESSAGE_BUFF);
	
	return NULL;
}
void* sending(void* sockNum) {
	char* message;
	unsigned int id = *((int*)sockNum);
	printf("\n\n%s: ", SERVER_NAME);
	scanf("%s", message);
	printf("\n%s: %s", SERVER_NAME, message);
	send(CLIENT[id].sockfd, message, strlen(message), 0);
	memset(message, 0, sizeof(message));
	
	return NULL;
}




