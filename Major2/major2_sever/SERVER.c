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

#define MSG_BUFF_LENGTH 32
#define SERVER_NAME "SERVER"

void  swapNames();
void* sendMyName();
void* getClientName();
void  createSocket();
void  setupProtocols(char*);
void  bindSocket();
void* listenAcceptSocket();
void  communicate();
void* sending();
void* receiving();

//======================================
//          GLOBAL VARIABLES
//======================================

sockaddr_in  SEVER_SETTINGS, CLIENT_SETTINGS;
pthread_t    TID[5];
pthread_t    LISTEN_THREAD;
hostent*     HOST;
fd_set       SOCKET_SET;
SOCKET       LOCAL_BIND_SOCKET;
SOCKET       CLIENT_SOCKET[2];
char         SEND_MESSAGE_BUFF[MSG_BUFF_LENGTH];
char         RECEIVE_MESSAGE_BUFF[MSG_BUFF_LENGTH];
char         CLIENT_NAME[20];
char*        HOSTNAME;
int          LOCAL_PORT;
int          NUM_CONNECT_CLIENTS = 0;
int          C;

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	if (argc < 3) 
		printf("\nUsage: <hostname> <port>...");
	
	HOSTNAME = argv[1];
	LOCAL_PORT = atoi(argv[2]);
	createSocket();
	setupProtocols(argv[1]);
	bindSocket();
	create_pthread(&LISTEN_THREAD, NULL, &listenAcceptSocket, NULL);
	acceptSocket();
	swapNames();
	communicate();

	close(LOCAL_BIND_SOCKET);
	close(CLIENT_SOCKET[0]);
	close(CLIENT_SOCKET[1]);

	return 0;
}
//=================================================================


void createSocket() {
	if ((LOCAL_BIND_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		int err = errno;
		printf("\nCount not create socket : %d", err);
		exit(1);
	} printf("\n>> Socket created.");
}

void setupProtocols(char* arg1) {
	memset(&HOST, 0, sizeof(HOST));
	HOST = gethostbyname(HOSTNAME);
	memset(&SEVER_SETTINGS, 0, sizeof(SEVER_SETTINGS));
	bcopy((char*)HOST->h_addr_list[0], (char*)&SEVER_SETTINGS.sin_addr.s_addr, HOST->h_length);
	SEVER_SETTINGS.sin_addr.s_addr = htonl(INADDR_ANY);
	SEVER_SETTINGS.sin_family = AF_INET;
	SEVER_SETTINGS.sin_port = htons(LOCAL_PORT);
	printf("\n>> Protocols created.");
}

void swapNames() {
	pthread_create(&TID[2], NULL, &getClientName, NULL);
	pthread_create(&TID[3], NULL, &sendMyName, NULL);
	pthread_join(TID[2], NULL);
	pthread_join(TID[3], NULL);
}

void* sendMyName() {
	send(CLIENT_SOCKET[0], SERVER_NAME, sizeof(CLIENT_NAME), 0);
	return NULL;
}
void* getClientName() {
	memset(CLIENT_NAME, 0, sizeof(CLIENT_NAME));
	recv(CLIENT_SOCKET[0], CLIENT_NAME, sizeof(CLIENT_NAME), 0);
	CLIENT_NAME[strlen(CLIENT_NAME)] = '\0';
	return NULL;
}



void bindSocket() {
	if ((bind(LOCAL_BIND_SOCKET, (struct sockaddr*)&SEVER_SETTINGS, sizeof(SEVER_SETTINGS))) < 0) {
		int errorNumber = errno;
		printf("\nBind failed with error code: %d", errorNumber);
		exit(1);
	}
	printf("\n>> Bind Succeeded");
}

void* listenAcceptSocket() {
	if ((listen(LOCAL_BIND_SOCKET, 2)) < 0) {
		int errorNumber = errno;
		printf("\n>> Listen failed, Error: %d", errorNumber);
		exit(1);
	}

	printf("\n>> Listening for incoming connections...");
	C = sizeof(struct sockaddr_in);

	if ((CLIENT_SOCKET[NUM_CONNECT_CLIENTS] = accept(LOCAL_BIND_SOCKET, (struct sockaddr*)&CLIENT_SETTINGS, &C)) < 0) {
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
			close(CLIENT_SOCKET[i]);
		}
		exit(2);
	}
	return NULL;
}

void communicate() {
	pthread_create(&TID[0], NULL, &receiving, NULL);
	pthread_create(&TID[1], NULL, &sending, NULL);
	pthread_join(TID[0], NULL);
	pthread_join(TID[1], NULL);
}

void* receiving() {
	int bytesReceived;
	while (1) {
		memset(RECEIVE_MESSAGE_BUFF, 0, sizeof(RECEIVE_MESSAGE_BUFF));
		bytesReceived = recv(CLIENT_SOCKET[0], RECEIVE_MESSAGE_BUFF, MSG_BUFF_LENGTH - 1, 0);
		RECEIVE_MESSAGE_BUFF[bytesReceived] = '\0';
		printf("\n\t%s: %s", CLIENT_NAME, RECEIVE_MESSAGE_BUFF);

	}
	return NULL;
}

void* sending() {
	char* message;
	while (1) {
		memset(message, 0, sizeof(message));
		printf("\n\n%s: ", SERVER_NAME);
		scanf("%s", message);
		printf("\n%s: %s", SERVER_NAME, message);
		send(CLIENT_SOCKET[0], message, strlen(message), 0);
	}
	return NULL;
}




