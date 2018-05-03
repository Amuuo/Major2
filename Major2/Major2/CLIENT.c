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

#define MSG_BUFF_LENGTH 128
typedef int SOCKET;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;

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
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
} server;

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

hostent*      HOST;
pthread_t     RECEIVE_THREAD;
pthread_t     SENDING_THREAD;
pthread_t     SEND_NAME;
pthread_t     GET_NAME;
client        CLIENT;
server        MAIN_SEVER;

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	if (argc < 4) {
		printf("\nUsage: <server hostname> <server port#> <client name>");
		exit(1);
	}
	MAIN_SEVER.port = atoi(argv[2]);
	strcpy(CLIENT.name, argv[3]);

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
	send(MAIN_SEVER.sockfd, CLIENT.name, sizeof(MAIN_SEVER.name), 0);

	return NULL;
}


void* getFriendName() {
	memset(&MAIN_SEVER.name[0], 0, sizeof(MAIN_SEVER.name));
	recv(MAIN_SEVER.sockfd, MAIN_SEVER.name, sizeof(MAIN_SEVER.name), 0);
	MAIN_SEVER.name[strlen(MAIN_SEVER.name)] = '\0';

	return NULL;
}

void createSocket() {
	if ((MAIN_SEVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n>> Count not create socket : \n");
		exit(1);
	} 
	printf("\n>> Socket created.");

	return;
}
void setupProtocols(char* arg1) {
	memset(&HOST, 0, sizeof(HOST));
	HOST = gethostbyname(arg1);
	memset(&MAIN_SEVER.protocols, 0, sizeof(MAIN_SEVER.protocols));
	bcopy((char*)HOST->h_addr_list[0], (char*)&MAIN_SEVER.protocols.sin_addr.s_addr, HOST->h_length);
	MAIN_SEVER.protocols.sin_family = AF_INET;
	MAIN_SEVER.protocols.sin_port = htons(MAIN_SEVER.port);
	printf("\n>> Protocols created.");

	return;
}

void connectSocket() {
	int check, tmp;
	check = connect(MAIN_SEVER.sockfd, (struct sockaddr*)&MAIN_SEVER.protocols, sizeof(MAIN_SEVER.protocols));
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
	
	while (1) {
		memset(message, 0, sizeof(message));
		printf("\n\n%s: ", CLIENT.name);
		scanf("%s", message);
		printf("\n\n%s: %s", CLIENT.name, message);
		if (sizeof(message) > 0) {
			send(MAIN_SEVER.sockfd, message, strlen(message), 0);
		}
	}

	return NULL;
}

void* receiving() {
	int bytesReceived;
	
	while (1) {
		memset(CLIENT.receive_msg_buff, 0, sizeof(CLIENT.receive_msg_buff));

		if ((bytesReceived = recv(MAIN_SEVER.sockfd, CLIENT.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0)) > 0) {
			CLIENT.receive_msg_buff[bytesReceived] = '\0';
			printf("\n\t%s: %s", MAIN_SEVER.name, CLIENT.receive_msg_buff);
		}
		if (CLIENT.receive_msg_buff == '0') {
			printf("\nReceived '0' from server, closing socket");
			close(CLIENT.sockfd);
		}
	}

	return NULL;
}
