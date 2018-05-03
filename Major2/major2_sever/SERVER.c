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

#define MSG_BUFF_LENGTH 256
#define NAME_SIZE 10
#define SERVER_NAME "SERVER"

typedef struct client {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[NAME_SIZE];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
} client;

typedef struct server {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[NAME_SIZE];
	char  send_msg_buff[MSG_BUFF_LENGTH];
	char  receive_msg_buff[MSG_BUFF_LENGTH];
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
void  clearStdin();
void  initializeMutexes();

//======================================
//          GLOBAL VARIABLES
//======================================

pthread_mutex_t RECEIVE_MUTEX;
pthread_mutex_t SENDING_MUTEX;
pthread_t    LISTEN_THREAD;
pthread_t    COMMUNICATE_THREAD;
pthread_t    RECEIVE_THREAD[2];
pthread_t    SENDING_THREAD[2];
pthread_t    SEND_NAME_THREAD;
pthread_t    GET_NAME_THREAD;
pthread_t    SWAP_THREAD;
hostent*     HOST;
client*      CLIENT = NULL;
server       MAIN_SERVER;
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
	MAIN_SERVER.port = atoi(argv[2]);
	*MAIN_SERVER.name = SERVER_NAME;
	createSocket();
	setupProtocols();
	bindSocket();
	pthread_create(&LISTEN_THREAD, NULL, &listenAcceptSocket, NULL);
	pthread_join(LISTEN_THREAD, NULL);
	//swapNames();
	//communicate();

	close(MAIN_SERVER.sockfd);
	close(CLIENT[0].sockfd);
	close(CLIENT[1].sockfd);

	return 0;
}
//=================================================================
//=================================================================

void createSocket() {
	if ((MAIN_SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
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
	memset(&MAIN_SERVER.protocols, 0, sizeof(MAIN_SERVER.protocols));
	bcopy((char*)HOST->h_addr_list[0], (char*)&MAIN_SERVER.protocols.sin_addr.s_addr, HOST->h_length);
	MAIN_SERVER.protocols.sin_addr.s_addr = htonl(INADDR_ANY);
	MAIN_SERVER.protocols.sin_family = AF_INET;
	MAIN_SERVER.protocols.sin_port = htons(MAIN_SERVER.port);
	printf("\n>> Protocols created.");

	return;
}
void* swapNames(void* sockNum) {
	pthread_create(&GET_NAME_THREAD, NULL, &getClientName, sockNum);
	pthread_create(&SEND_NAME_THREAD, NULL, &sendMyName, sockNum);
	pthread_join(GET_NAME_THREAD, NULL);
	pthread_join(SEND_NAME_THREAD, NULL);

	return NULL;
}

void* sendMyName(void* sockNum) {
	unsigned int id = *((int*)sockNum);
	unsigned int nameSize = sizeof(MAIN_SERVER.name);

	send(CLIENT[id].sockfd, MAIN_SERVER.name, nameSize, 0);

	return NULL;
}
void* getClientName(void* sockNum) {
	unsigned int id = *((int*)sockNum);	
	recv(CLIENT[id].sockfd, CLIENT[id].name, NAME_SIZE, 0);
	CLIENT[id].name[strlen(CLIENT[id].name)] = '\0';

	return NULL;
}
void bindSocket() {
	if ((bind(MAIN_SERVER.sockfd, (sockaddr*)&MAIN_SERVER.protocols, sizeof(MAIN_SERVER.protocols))) < 0) {
		int errorNumber = errno;
		printf("\nBind failed with error code: %d", errorNumber);
		exit(1);
	}
	printf("\n>> Bind Succeeded");

	return;
}
void* listenAcceptSocket() {
	unsigned int sockNum;
	while (NUM_CONNECT_CLIENTS < 2) {

		if ((listen(MAIN_SERVER.sockfd, 2)) < 0) {
			int errorNumber = errno;
			printf("\n>> Listen failed, Error: %d", errorNumber);
			exit(1);
		}

		printf("\n>> Listening for incoming connections...");
		SOCKADDR_IN_SIZE = sizeof(struct sockaddr_in);

		if ((CLIENT[NUM_CONNECT_CLIENTS].sockfd = accept(MAIN_SERVER.sockfd, 
			(sockaddr*)&CLIENT[NUM_CONNECT_CLIENTS].protocols, &SOCKADDR_IN_SIZE)) < 0) {
			int errorNumber = errno;
			printf("\nAccept failed with error code: %d", errorNumber);
			exit(1);
		} 
		++NUM_CONNECT_CLIENTS;
		sockNum = NUM_CONNECT_CLIENTS - 1;

		pthread_create(&SWAP_THREAD, NULL, &swapNames, (void*)&sockNum);
		pthread_join(SWAP_THREAD, NULL);
		printf("\n>> Connection accepted.\n\n");


		// exit if more than 2 clients attempt to connect
		if (NUM_CONNECT_CLIENTS > 2) {
			printf("\nMore than 2 clients connected. Exiting");
			int i;
			for (i = 0; i < 3; ++i) {
				close(CLIENT[i].sockfd);
			}
			exit(2);
		}
		pthread_create(&RECEIVE_THREAD[sockNum], NULL, &receiving, (void*)&sockNum);
		pthread_create(&SENDING_THREAD[sockNum], NULL, &sending, (void*)&sockNum);
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
		pthread_mutex_lock(&RECEIVE_MUTEX);
		printf("\nReceiving TID: %ld", RECEIVE_THREAD[id]);
		memset(MAIN_SERVER.receive_msg_buff, 0, sizeof(MAIN_SERVER.receive_msg_buff));
		//clearStdin();
		printf("\nWaiting for public key from first client...");
		bytesReceived = recv(CLIENT[id].sockfd, MAIN_SERVER.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0);
		printf("\nMessage received: %s", MAIN_SERVER.receive_msg_buff);
		if (/*Recieved prime numbers 'p' and 'q' are valid*/ 1) {

			/*generate public key (n,d), store in MAIN_SERVER.n, and MAIN_SERVER.d*/

		}
		MAIN_SERVER.receive_msg_buff[bytesReceived] = '\0';
		printf("\n%s: %s", CLIENT[id].name, MAIN_SERVER.receive_msg_buff);
		pthread_mutex_unlock(&RECEIVE_MUTEX);
	}
	return NULL;
}
void* sending(void* sockNum) {
	char* message;
	unsigned int id = *((int*)sockNum);
	
	while (1) {
		pthread_mutex_lock(&SENDING_MUTEX);
		message = (char*)malloc(MSG_BUFF_LENGTH);
		printf("\nSending TID: %ld", SENDING_THREAD[id]);
		printf("\n\n%s: ", SERVER_NAME);
		fgets(message, MSG_BUFF_LENGTH, stdin);
		if (message != NULL) {
			send(CLIENT[id].sockfd, message, strlen(message), 0);
		}
		if (message == '0') {
			--NUM_CONNECT_CLIENTS;
		}
		free(message);
		clearStdin();
		pthread_mutex_lock(&SENDING_MUTEX);
	}
	
	return NULL;
}
void clearStdin() {
	int tmp;
	do {
		tmp = getchar();
	} while (tmp != EOF && tmp != '\n');
}

void initializeMutexes() {
	pthread_mutex_init(&RECEIVE_MUTEX, NULL);
	pthread_mutex_init(&SENDING_MUTEX, NULL);
	return;
}




