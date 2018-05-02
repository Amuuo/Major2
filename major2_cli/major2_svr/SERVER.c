#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<termio.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netdb.h>
#include<fcntl.h>
#include<errno.h>


#define MSG_BUFF_LENGTH 32

typedef int SOCKET;


void swapNames();
void* sendMyName();
void* getClientName();
void createSocket();
void setupProtocols(char*);
void bindSocket();
void listenToSocket();
void acceptSocket();
void communicate();
void* sending();
void* receiving();
void selectSocket();

char SEND_MESSAGE_BUFF[MSG_BUFF_LENGTH];
char RECEIVE_MESSAGE_BUFF[MSG_BUFF_LENGTH];
char CLIENT_NAME[20];
const char* SVR_NAME = "Server";
int  c;
struct hostent* HOST;
struct sockaddr_in  SEVER_SETTINGS, CLIENT_SETTINGS;
SOCKET LOCAL_BIND_SOCKET, CLIENT_SOCKET[2];
int LOCAL_PORT;
char* HOSTNAME;
pthread_t TID[5];
fd_set SOCKET_SET;


int main(int argc, char* argv[]) {

	if (argc < 3) {
		printf("\nUsage: <hostname> <port>...");
	}
	HOSTNAME = argv[1];
	LOCAL_PORT = atoi(argv[2]);
	createSocket();
	setupProtocols(argv[1]);
	bindSocket();
	listenToSocket();
	acceptSocket();
	swapNames();
	communicate();

	close(LOCAL_BIND_SOCKET);
	close(CLIENT_SOCKET[0]);
	close(CLIENT_SOCKET[1]);

	return 0;
}
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
	send(CLIENT_SOCKET[0], SVR_NAME, sizeof(CLIENT_NAME), 0);
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

void listenToSocket() {
	if ((listen(LOCAL_BIND_SOCKET, 2)) < 0) {
		int errorNumber = errno;
		printf("\n>> Listen failed, Error: %d", errorNumber);
		exit(1);
	}

	printf("\n>> Listening for incoming connections...");
	c = sizeof(struct sockaddr_in);
}

void acceptSocket() {
	if ((CLIENT_SOCKET[0] = accept(LOCAL_BIND_SOCKET, (struct sockaddr*)&CLIENT_SETTINGS, &c)) < 0) {
		int errorNumber = errno;
		printf("\nAccept failed with error code: %d", errorNumber);
		exit(1);
	} printf("\n>> Connection accepted.\n\n");
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

void selectSocket() {
	select(2, &SOCKET_SET, NULL, NULL, NULL);
}


void* sending() {
	char* message;
	while (1) {
		memset(message, 0, sizeof(message));
		printf("\n\n%s: ", SVR_NAME);
		scanf("%s", message);
		printf("\n%s: %s", SVR_NAME, message);
		send(CLIENT_SOCKET[0], message, strlen(message), 0);
	}
	return NULL;
}




