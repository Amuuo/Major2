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
#include <math.h>

typedef int    SOCKET;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct sockaddr sockaddr;

#define MSG_BUFF_LENGTH 256
#define NAME_SIZE 10
#define SERVER_NAME "SERVER"

typedef struct {
	SOCKET sockfd;
	sockaddr_in protocols;
	sockaddr_in that_client_protocols;
	char name[NAME_SIZE];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	char* hostname;
	unsigned int port;
} client;

typedef struct {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[NAME_SIZE];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int port;
	unsigned int n;
	unsigned int d;
	unsigned int e;
} server;

void*  swapNames(void*);
void* sendMyName(void*);
void* getClientName(void*);
void  createSocket();
void  setupProtocols();
void  bindSocket();
void* listenAcceptSocket();
void* handleClients(void*);
void  initializeMutexes();

//======================================
//          GLOBAL VARIABLES
//======================================

pthread_mutex_t RECEIVE_MUTEX;
pthread_mutex_t SENDING_MUTEX;
pthread_cond_t  RECEIVE_CONDITION;
pthread_cond_t  LISTEN_CONDITION;
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

	pthread_cond_init(&RECEIVE_CONDITION, NULL);
	pthread_cond_init(&LISTEN_CONDITION, NULL);

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
		printf("\nCount not create socket : %d", errno);
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
		printf("\nBind failed with error code: %d", errno);
		exit(1);
	}
	printf("\n>> Bind Succeeded");

	return;
}
void* listenAcceptSocket() {
	unsigned int sockNum;
	while (1) {

		if ((listen(MAIN_SERVER.sockfd, 2)) < 0) {			
			printf("\n>> Listen failed, Error: %d", errno);
			exit(1);
		}

		printf("\n>> Listening for incoming connections...");
		SOCKADDR_IN_SIZE = sizeof(sockaddr_in);

		if ((CLIENT[NUM_CONNECT_CLIENTS].sockfd = accept(MAIN_SERVER.sockfd, 
			(sockaddr*)&CLIENT[NUM_CONNECT_CLIENTS].protocols, &SOCKADDR_IN_SIZE)) < 0) {			
			printf("\nAccept failed with error code: %d", errno);
			exit(1);
		} 
		printf("\nClient[%d] connected", NUM_CONNECT_CLIENTS);
		// get host info from client, to eventually hand of to CLIENT2
		//getpeername(CLIENT[NUM_CONNECT_CLIENTS].sockfd, (sockaddr*)&CLIENT[NUM_CONNECT_CLIENTS].protocols, sizeof(sockaddr));
		++NUM_CONNECT_CLIENTS;
		sockNum = NUM_CONNECT_CLIENTS - 1;

		pthread_create(&SWAP_THREAD, NULL, &swapNames, (void*)&sockNum);
		pthread_join(SWAP_THREAD, NULL);
		printf("\n>> Connection accepted.\n\n");


		// exit if more than 2 clients attempt to connect
		if (NUM_CONNECT_CLIENTS > 2) {
				printf("\nMore than 2 clients connected. Disconnecting");						
				close(CLIENT[NUM_CONNECT_CLIENTS-1].sockfd);				
		}
		if (NUM_CONNECT_CLIENTS == 2) {
			pthread_create(&RECEIVE_THREAD[sockNum], NULL, &handleClients, (void*)&sockNum);
			NUM_CONNECT_CLIENTS = 0;
		}
	}
	return NULL;
}

void* handleClients(void* sockNum) {
	unsigned int bytesReceived;
	unsigned int id = *((int*)sockNum);
	
	while (1) {
		pthread_mutex_lock(&RECEIVE_MUTEX);
		memset(MAIN_SERVER.receive_msg_buff, 0, sizeof(MAIN_SERVER.receive_msg_buff));
		printf("\nWaiting for public key from first client...");
		bytesReceived = recv(CLIENT[0].sockfd, MAIN_SERVER.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0);
		printf("\nMessage received: %s", MAIN_SERVER.receive_msg_buff);
		if (/*Recieved prime numbers 'p' and 'q' are valid*/ 1) {

			/********************************************************************
			 generate public key (n,d), store in MAIN_SERVER.n, and MAIN_SERVER.d
			 *********************************************************************/
		}
		else {
			char* msg = "\nINVALID";
			send(CLIENT[0].sockfd, msg, strlen(msg), 0);
			continue;
		}
		// generate private key and send to CLIENT1

			char* private_key;

		/*******************************************************
		 generate public key and send to CLIENT[abs(id-1)] as 
		 string in the format "KEY n e" 
		 *******************************************************/
			
		send(CLIENT[abs(id - 1)].sockfd, private_key, strlen(private_key), 0);

		/*******************************************************
		 follow up message with another message containing 
		 CLIENT[abs(id-1)].protocols, so CLIENT[id] can connect
		 *******************************************************/
		
		// if there's not 2 clients connected, signal client to disconnect with '0'
		else {
			char* msg = '0';
			send(CLIENT[0].sockfd, msg, strlen(msg), 0);
			return NULL;
		}
		MAIN_SERVER.receive_msg_buff[bytesReceived] = '\0';
		printf("\n%s: %s", CLIENT[0].name, MAIN_SERVER.receive_msg_buff);
		pthread_mutex_unlock(&RECEIVE_MUTEX);
	}
	return NULL;
}
void initializeMutexes() {
	pthread_mutex_init(&RECEIVE_MUTEX, NULL);
	pthread_mutex_init(&SENDING_MUTEX, NULL);
	return;
}




