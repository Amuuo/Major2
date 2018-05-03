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

/*
typedef struct streams {
	FILE* RECEIVE_OUT_STREAM;
	FILE* RECEIVE_IN_STREAM;
	FILE* SENDING_OUT_STREAM;
	FILE* SENDING_IN_STREAM;
} Streams;
*/

typedef struct client {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
	//Streams filestreams;
} Client;

typedef struct server {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
	//Streams filestreams;
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
void  clearStdin();
//void  initializeFileStreams(void*, char);

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
Server          MAIN_SERVER;
/*
FILE*           RECEIVE_OUT_STREAM;
FILE*           RECEIVE_IN_STREAM;
FILE*           SENDING_OUT_STREAM;
FILE*           SENDING_IN_STREAM;
*/
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
	initializeFileStreams(&CLIENT, "C");

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
void setupAsSever() {

	return;
}
void* sending() {
	char* message;
	
	while (1) {
		printf("\nSending TID: %ld", SENDING_THREAD);
		printf("\n\n%s, enter public key (p q): ", CLIENT.name);
		//scanf("%s", message);
		fgets(CLIENT.send_msg_buff, MSG_BUFF_LENGTH, stdin);
		
		if (sizeof(message) > 0) {
			send(MAIN_SERVER.sockfd, CLIENT.send_msg_buff, MSG_BUFF_LENGTH, 0);
		}
		memset(message, 0, sizeof(message));
		clearStdin();
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

void clearStdin() {
	int tmp;
	do {
		tmp = getchar();
	} while (tmp != EOF && tmp != '\n');
}

/*
void initializeFileStreams(void* tmpObj, char id) {
	if (id == 'C' || id == 'c') {
		((Client*)tmpObj)->filestreams.RECEIVE_OUT_STREAM = fopen(strcat(((Client*)tmpObj)->name, "RECEIVE_OUT_STREAM"), "a+");
		((Client*)tmpObj)->filestreams.RECEIVE_IN_STREAM  = fopen(strcat(((Client*)tmpObj)->name, "RECEIVE_IN_STREAM"), "a+");
		((Client*)tmpObj)->filestreams.SENDING_OUT_STREAM = fopen(strcat(((Client*)tmpObj)->name, "SENDING_OUT_STREAM"), "a+");
		((Client*)tmpObj)->filestreams.SENDING_IN_STREAM  = fopen(strcat(((Client*)tmpObj)->name, "SENDING_IN_STREAM"), "a+");
	}
	else if (id == 'S' || id == 'S') {
		((Server*)tmpObj)->filestreams.RECEIVE_OUT_STREAM = fopen(strcat(((Server*)tmpObj)->name, "RECEIVE_OUT_STREAM"), "a+");
		((Server*)tmpObj)->filestreams.RECEIVE_IN_STREAM  = fopen(strcat(((Server*)tmpObj)->name, "RECEIVE_IN_STREAM"), "a+");
		((Server*)tmpObj)->filestreams.SENDING_OUT_STREAM = fopen(strcat(((Server*)tmpObj)->name, "SENDING_OUT_STREAM"), "a+");
		((Server*)tmpObj)->filestreams.SENDING_IN_STREAM  = fopen(strcat(((Server*)tmpObj)->name, "SENDING_IN_STREAM"), "a+");
	}
}
*/