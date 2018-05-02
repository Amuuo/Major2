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
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;

void* receiving();
void* sending();
void* sendMyName();
void* getFriendName();
void  swapNames();
void  createSocket();
void  setupProtocols(char*);
void  connectSocket();
void  communicate();
void  setupAsSever();

//======================================
//          GLOBAL VARIABLES
//======================================

sockaddr_in  SERVER;
hostent*     HOST;
pthread_t    RECEIVE_THREAD;
pthread_t    SENDING_THREAD;
pthread_t    SEND_NAME;
pthread_t    GET_NAME;
SOCKET       SERVER_SOCKET;
char*        CLIENT1_NAME;
char         SERVER_NAME[20];
char         SEND_MESSAGE_BUFFER[MSG_BUFF_LENGTH];
char         RECEIVE_MESSAGE_BUFFER[MSG_BUFF_LENGTH];
int          SERVER_PORT;
int          NEW_CLIENT_PORT;


//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	if (argc < 4) {
		printf("\nUsage: <server hostname> <server port#> <client name>");
		exit(1);
	}
	SERVER_PORT = atoi(argv[2]);
	CLIENT1_NAME = argv[3];

	createSocket();
	setupProtocols(argv[1]);
	connectSocket();
	swapNames();
	communicate();

	close(SERVER_SOCKET);

	return 0;
}
//=================================================================

void swapNames() {
	pthread_create(&SEND_NAME, NULL, &sendMyName, NULL);
	pthread_create(&GET_NAME, NULL, &getFriendName, NULL);
	pthread_join(SEND_NAME, NULL);
	pthread_join(GET_NAME, NULL);

	return;
}

void* sendMyName() {
	send(SERVER_SOCKET, CLIENT1_NAME, sizeof(SERVER_NAME), 0);

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
	} 
	printf("\n>> Socket created.");

	return;
}
void setupProtocols(char* arg1) {
	memset(&HOST, 0, sizeof(HOST));
	HOST = gethostbyname(arg1);
	memset(&SERVER, 0, sizeof(SERVER));
	bcopy((char*)HOST->h_addr_list[0], (char*)&SERVER.sin_addr.s_addr, HOST->h_length);
	SERVER.sin_family = AF_INET;
	SERVER.sin_port = htons(SERVER_PORT);
	printf("\n>> Protocols created.");

	return;
}

void connectSocket() {
	int check, tmp;
	check = connect(SERVER_SOCKET, (struct sockaddr*)&SERVER, sizeof(SERVER));
	if (check < 0) {
		tmp = errno;
		printf("\n>> check = %d", check);
		printf("\n>> Connect failed.. Error: %d\n", tmp);
		exit(1);
	} 
	printf("\n>> Socket connected\n\n");

	return;
}

void communicate() {
	pthread_create(&RECEIVE_THREAD, NULL, &receiving, NULL);
	pthread_create(&SENDING_THREAD, NULL, &sending, NULL);
	pthread_join(RECEIVE_THREAD, NULL);
	pthread_join(SENDING_THREAD, NULL);

	return;
}

void setupAsSever() {

	return;
}

void* sending() {
	char* message;
	memset(message, 0, sizeof(message));
	printf("\n\n%s: ", CLIENT1_NAME);
	scanf("%s", message);
	printf("\n\n%s: %s", CLIENT1_NAME, message);
	send(SERVER_SOCKET, message, strlen(message), 0);

	return NULL;
}

void* receiving() {
	int bytesReceived;
	memset(RECEIVE_MESSAGE_BUFFER, 0, sizeof(RECEIVE_MESSAGE_BUFFER));
	bytesReceived = recv(SERVER_SOCKET, RECEIVE_MESSAGE_BUFFER, MSG_BUFF_LENGTH - 1, 0);
	RECEIVE_MESSAGE_BUFFER[bytesReceived] = '\0';
	printf("\n\t%s: %s", SERVER_NAME, RECEIVE_MESSAGE_BUFFER);

	return NULL;
}
