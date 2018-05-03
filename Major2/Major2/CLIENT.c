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

#define MSG_BUFF_LENGTH 256
typedef int SOCKET;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct sockaddr sockaddr;

typedef struct {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;	
} Client;

typedef struct {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
} Server;

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
void  connectToClient2();

//======================================
//          GLOBAL VARIABLES
//======================================

hostent*        HOST;
pthread_t       RECEIVE_THREAD;
pthread_t       SENDING_THREAD;
pthread_t       SEND_NAME_THREAD;
pthread_t       GET_NAME_THREAD;
pthread_mutex_t MUTEX;
Client          CLIENT;
Client          CLIENT2;
Server          MAIN_SERVER;
unsigned int    P;
unsigned int    Q;
unsigned int    E;
unsigned int    N;

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	pthread_mutex_init(&MUTEX, NULL);
	if (argc < 4) {
		printf("\nUsage: <server hostname> <server port#> <client name>");
		exit(1);
	}
	MAIN_SERVER.port = atoi(argv[2]);
	strcpy(CLIENT.name, argv[3]);

	createSocket();
	setupProtocols(argv[1]);
	connectSocket();
	swapNames();
	communicate();

	close(MAIN_SERVER.sockfd);

	return 0;
}
//=================================================================

void swapNames() {
	pthread_create(&SEND_NAME_THREAD, NULL, &sendMyName, NULL);
	pthread_create(&GET_NAME_THREAD, NULL, &getFriendName, NULL);
	pthread_join(SEND_NAME_THREAD, NULL);
	pthread_join(GET_NAME_THREAD, NULL);

	return;
}
void* sendMyName() {
	send(MAIN_SERVER.sockfd, CLIENT.name, sizeof(MAIN_SERVER.name), 0);

	return NULL;
}
void* getFriendName() {
	memset(&MAIN_SERVER.name[0], 0, sizeof(MAIN_SERVER.name));
	recv(MAIN_SERVER.sockfd, MAIN_SERVER.name, sizeof(MAIN_SERVER.name), 0);
	MAIN_SERVER.name[strlen(MAIN_SERVER.name)] = '\0';

	return NULL;
}
void createSocket() {
	if ((MAIN_SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n>> Count not create socket : \n");
		exit(1);
	} 
	printf("\n>> Socket created.");

	return;
}
void setupProtocols(char* arg1) {
	memset(&HOST, 0, sizeof(HOST));
	HOST = gethostbyname(arg1);
	memset(&MAIN_SERVER.protocols, 0, sizeof(MAIN_SERVER.protocols));
	bcopy((char*)HOST->h_addr_list[0], (char*)&MAIN_SERVER.protocols.sin_addr.s_addr, HOST->h_length);
	MAIN_SERVER.protocols.sin_family = AF_INET;
	MAIN_SERVER.protocols.sin_port = htons(MAIN_SERVER.port);
	printf("\n>> Protocols created.");

	return;
}
void connectSocket() {
	int check, tmp;
	check = connect(MAIN_SERVER.sockfd, (struct sockaddr*)&MAIN_SERVER.protocols, sizeof(MAIN_SERVER.protocols));
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
void* sending() {
	char* message;
	int bytesReceived;
	char c;
	while (1) {
		printf("\nSending TID: %ld", SENDING_THREAD);
		printf("\n\n%s, enter prime numbers key (p q): ", CLIENT.name);
		//scanf("%s", message);
		fgets(CLIENT.send_msg_buff, MSG_BUFF_LENGTH, stdin);
		
		if (sizeof(message) > 0) {
			send(MAIN_SERVER.sockfd, CLIENT.send_msg_buff, MSG_BUFF_LENGTH, 0);
		}
		if (bytesReceived = (recv(MAIN_SERVER.sockfd, CLIENT.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0)) < 0) {
			int errorNum = errno;
			printf("\nError on receive: %d", errorNum);
			exit(2);
		}
		CLIENT.receive_msg_buff[bytesReceived] = '\0';

		FILE* server_msg = fopen("server_msg", "a+");
		while ((c = fgetc(CLIENT.receive_msg_buff)) != '\0') {
			fputc(c, server_msg);
		}
		rewind(server_msg);
		char* tmp;
		if (fscanf(server_msg, "%s", tmp) == "0") {
			printf("\nReceived '0' from Main Server, disconnecting...");
			close (MAIN_SERVER.sockfd);
			exit(3);
		}
		else if (tmp == "KEY") {
			sockaddr_in* tmpAddr;
			recv(MAIN_SERVER.sockfd, (sockaddr*)tmpAddr, sizeof(sockaddr), 0);
			CLIENT2.protocols = *tmpAddr;
			sleep(1);
			connectToClient2();
		}
		memset(message, 0, sizeof(message));
	}

	return NULL;
}
void* receiving() {
	int bytesReceived;
	
	while (1) {
		printf("\nReceive TID: %ld", RECEIVE_THREAD);
		memset(CLIENT.receive_msg_buff, 0, sizeof(CLIENT.receive_msg_buff));

		if ((bytesReceived = recv(MAIN_SERVER.sockfd, CLIENT.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0)) > 0) {
			CLIENT.receive_msg_buff[bytesReceived] = '\0';
			printf("\n%s: %s", MAIN_SERVER.name, CLIENT.receive_msg_buff);
		}
		if (CLIENT.receive_msg_buff == '0') {
			printf("\nReceived '0' from server, closing socket");
			close(CLIENT.sockfd);
		}
	}

	return NULL;
}
void setupAsSever() {

	return;
}

void connectToClient2() {
	int check;
	if ((connect(CLIENT2.sockfd, (sockaddr*)&CLIENT2.protocols, sizeof(CLIENT2.protocols))) < 0) {
		int errorNum = errno;
		printf("\nCould not connect with CLIENT2, Error: %d", errorNum);
		exit(2);
	}
}