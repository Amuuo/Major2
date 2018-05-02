#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <termio.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>

#define MSG_BUFF_LENGTH 32
typedef int SOCKET;

void* receiving();
void* sending();
void* sendMyName();
void* getFriendName();
void  swapNames();
void  createSocket();
void  setupProtocols(char*);
void  connectSocket();
void  communicate();

//======================================
//          GLOBAL VARIABLES
//======================================

struct sockaddr_in  SERVER;
struct hostent*     HOST;
pthread_t           TID[4];
SOCKET              SERVER_SOCKET;
char*               CLIENT_NAME;
char                SERVER_NAME[20];
char                SEND_MESSAGE_BUFFER[MSG_BUFF_LENGTH];
char                RECEIVE_MESSAGE_BUFFER[MSG_BUFF_LENGTH];
int                 SERVER_PORT;

//======================================

int main(int argc, char* argv[]) {

	if (argc < 4) {
		printf("\nUsage: <server hostname> <server port#> <client name>");
	}
	SERVER_PORT = atoi(argv[2]);
	CLIENT_NAME = argv[3];
	createSocket();
	setupProtocols(argv[1]);
	connectSocket();
	swapNames();
	communicate();

	close(SERVER_SOCKET);

	return 0;
}


void swapNames() {
	pthread_create(&TID[2], NULL, &sendMyName, NULL);
	pthread_create(&TID[3], NULL, &getFriendName, NULL);
	pthread_join(TID[2], NULL);
	pthread_join(TID[3], NULL);
}

void* sendMyName() {
	send(SERVER_SOCKET, CLIENT_NAME, sizeof(SERVER_NAME), 0);
	return NULL;
}


void* getFriendName() {
	memset(&SERVER_NAME[0], 0, sizeof(SERVER_NAME));
	recv(SERVER_SOCKET, SERVER_NAME, sizeof(SERVER_NAME), 0);
	SERVER_NAME[strlen(SERVER_NAME)] = '\0';
	return NULL;
}

void createSocket() {
	if ((SERVER_SOCKET = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n>> Count not create socket : \n");
		exit(1);
	} printf("\n>> Socket created.");
}
void setupProtocols(char* arg1) {
	memset(&HOST, 0, sizeof(HOST));
	HOST = gethostbyname(arg1);
	memset(&SERVER, 0, sizeof(SERVER));
	bcopy((char*)HOST->h_addr_list[0], (char*)&SERVER.sin_addr.s_addr, HOST->h_length);
	SERVER.sin_family = AF_INET;
	SERVER.sin_port = htons(SERVER_PORT);
	printf("\n>> Protocols created.");
}

void connectSocket() {
	int check, tmp;
	check = connect(SERVER_SOCKET, (struct sockaddr*)&SERVER, sizeof(SERVER));
	if (check < 0) {
		tmp = errno;
		printf("\n>> check = %d", check);
		printf("\n>> Connect failed.. Error: %d\n", tmp);
		exit(1);
	} printf("\n>> Socket connected\n\n");
}

void communicate() {
	pthread_create(&TID[0], NULL, &receiving, NULL);
	pthread_create(&TID[1], NULL, &sending, NULL);
	pthread_join(TID[0], NULL);
	pthread_join(TID[1], NULL);
}

void* sending() {
	char* message;
	while (1) {
		memset(message, 0, sizeof(message));
		printf("\n\n%s: ", CLIENT_NAME);
		scanf("%s", message);
		printf("\n\n%s: %s", CLIENT_NAME, message);
		send(SERVER_SOCKET, message, strlen(message), 0);
	}
	return NULL;
}

void* receiving() {
	int bytesReceived;
	while (1) {
		memset(RECEIVE_MESSAGE_BUFFER, 0, sizeof(RECEIVE_MESSAGE_BUFFER));
		bytesReceived = recv(SERVER_SOCKET, RECEIVE_MESSAGE_BUFFER, MSG_BUFF_LENGTH - 1, 0);
		RECEIVE_MESSAGE_BUFFER[bytesReceived] = '\0';
		printf("\n\t%s: %s", SERVER_NAME, RECEIVE_MESSAGE_BUFFER);
	}
	return NULL;
}
