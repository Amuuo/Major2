#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
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
} Client;

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
} Server;

void  createSocket();
void  setupProtocols();
void  bindSocket();
void* listenAcceptSocket();
void* handleClients(void*);
void  initializeMutexes();
int*  createPublicKey();
int*  createPrivateKey();

//======================================
//          GLOBAL VARIABLES
//======================================

pthread_mutex_t RECEIVE_MUTEX;
pthread_mutex_t SENDING_MUTEX;
pthread_cond_t  RECEIVE_CONDITION;
pthread_cond_t  LISTEN_CONDITION;
pthread_t    LISTEN_THREAD;
pthread_t    LISTEN_THREAD2;
pthread_t    COMMUNICATE_THREAD;
pthread_t    RECEIVE_THREAD[2];
pthread_t    SENDING_THREAD[2];
pthread_t    SEND_NAME_THREAD;
pthread_t    GET_NAME_THREAD;
pthread_t    SWAP_THREAD;
hostent*     HOST;
Client       CLIENT[2];
Server       MAIN_SERVER;
char*        HOSTNAME;
int          NUM_CONNECT_CLIENTS = 0;
int          SOCKADDR_IN_SIZE;
unsigned int INTPAIR[2];

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {
	system("clear");
	pthread_cond_init(&RECEIVE_CONDITION, NULL);
	pthread_cond_init(&LISTEN_CONDITION, NULL);

	printf("\n=========================================");
	printf("\n              S E R V E R");
	printf("\n=========================================\n");
	if (argc < 3) 
		printf("\nUsage: <hostname> <port>...");
	
	HOSTNAME = argv[1];
	MAIN_SERVER.port = atoi(argv[2]);
	createSocket();
	setupProtocols();
	bindSocket();
	pthread_create(&LISTEN_THREAD, NULL, &listenAcceptSocket, NULL);
	pthread_join(LISTEN_THREAD, NULL);

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
void bindSocket() {
	if ((bind(MAIN_SERVER.sockfd, (sockaddr*)&MAIN_SERVER.protocols, sizeof(MAIN_SERVER.protocols))) < 0) {		
		printf("\n>> Bind failed with error code: %d", errno);
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
		socklen_t sockSize= sizeof(sockaddr);

		if ((CLIENT[NUM_CONNECT_CLIENTS].sockfd = accept(MAIN_SERVER.sockfd, (sockaddr*)&CLIENT[NUM_CONNECT_CLIENTS].protocols, &sockSize)) < 0) {			
			printf("\n>> Accept failed with error code: %d", errno);
			exit(1);
		} 
		printf("\n>> Client[%d] connected", NUM_CONNECT_CLIENTS);
				
		++NUM_CONNECT_CLIENTS;
		sockNum = NUM_CONNECT_CLIENTS - 1;

		printf("\n>> Connection accepted.\n\n");
		
		if (NUM_CONNECT_CLIENTS > 2) {
				printf("\n>> More than 2 clients connected. Disconnecting");						
				close(CLIENT[sockNum].sockfd);				
		}
		if (NUM_CONNECT_CLIENTS == 2) {			
			pthread_create(&RECEIVE_THREAD[1], NULL, &handleClients, (void*)&sockNum);
			NUM_CONNECT_CLIENTS = 0;
			pthread_create(&LISTEN_THREAD2, NULL, &listenAcceptSocket, NULL);
			pthread_join(RECEIVE_THREAD[1], NULL);
		}
	}
	return NULL;
}

void* handleClients(void* sockNum) {
	unsigned int bytesReceived;
	unsigned int id = *((int*)sockNum);
	Client* tmpCli = malloc(sizeof(Client)*2);
	memcpy(tmpCli, CLIENT, sizeof(Client) * 2);
	memset(CLIENT, NULL, sizeof(Client) * 2);
	
	//pthread_mutex_lock(&RECEIVE_MUTEX);
	memset(MAIN_SERVER.receive_msg_buff, 0, MSG_BUFF_LENGTH);
	memset(MAIN_SERVER.send_msg_buff, 0, MSG_BUFF_LENGTH);

	printf("\n>> Waiting for public key from first client...");

	// send CLIENT[0] a msg to send back 2 prime numbers
	do {
		memset(INTPAIR, NULL, sizeof(unsigned int) * 2);
		strcpy(MAIN_SERVER.send_msg_buff, "Send 2 prime numbers (p,q)\0");
		if ((send(tmpCli[0].sockfd, MAIN_SERVER.send_msg_buff, MSG_BUFF_LENGTH, 0)) < 0) {
			printf("\n>> Failed to send message \"%s\" to CLIENT[0]", MAIN_SERVER.send_msg_buff);
			printf(", Error: %d", errno);
			exit(2);
		}

		if ((bytesReceived = recv(tmpCli[0].sockfd, (unsigned int*)INTPAIR, sizeof(unsigned int) * 2, 0)) < 0) {
			printf("\n>> Failed to receive prime numbers from CLIENT[0], Error: %d", errno);
			close(tmpCli[0].sockfd);
			close(tmpCli[1].sockfd);
			exit(1);
		}
		printf("\n>> Primes received from CLIENT[0]: %d, %d", INTPAIR[0], INTPAIR[1]);
		if (INTPAIR[0] * INTPAIR[1] <= 128) {
			printf("\n\n>> ( %d * %d = %d ) <= 128 \n\t ****DENIED****", INTPAIR[0], INTPAIR[1], INTPAIR[0]*INTPAIR[1]);
			printf("\n\n>> Sending CLIENT[0] message to resubmit");
			memset(MAIN_SERVER.send_msg_buff, 0, MSG_BUFF_LENGTH);
			strcpy(MAIN_SERVER.send_msg_buff, "Denied");
			send(tmpCli[0].sockfd, MAIN_SERVER.send_msg_buff, MSG_BUFF_LENGTH, 0);
		}
	} while (INTPAIR[0] * INTPAIR[1] <= 128);
	
	memset(MAIN_SERVER.send_msg_buff, 0, MSG_BUFF_LENGTH);
	memcpy(MAIN_SERVER.send_msg_buff, "Accepted", strlen("Accepted"));
	if ((send(tmpCli[0].sockfd, MAIN_SERVER.send_msg_buff, MSG_BUFF_LENGTH, 0)) < 0) {
		printf("\n>> Accepted messaged did not get send to CLIENT[0], Error: %d", errno);
		close(tmpCli[0].sockfd);
		close(tmpCli[1].sockfd);
		exit(1);
	}
	printf("\n>> Accepted primes from CLIENT[0]");

			
	//while (/*Recieved prime numbers 'p' and 'q' are not valid*/ 1) {
	//	char* msg = "\nINVALID";
	//	send(CLIENT[0].sockfd, msg, strlen(msg), 0);
	//	continue;
	//}
	/********************************************************************
	generate public key (n,d), send to CLIENT[1], format: "KEY n d"
	place result in MAIN_SERVER.send_msg_buff
	*********************************************************************/
	/*char* tmp = "KEY ";
	char* tmp2;
	char* tmp3;
	itoa(INTPAIR[0], tmp2, sizeof(int));
	itoa(INTPAIR[1], tmp3, sizeof(int));
	strcat(tmp, tmp2);
	strcat(tmp, tmp3);*/
	memset(tmpCli[1].send_msg_buff, NULL, MSG_BUFF_LENGTH);
	strcpy(tmpCli[1].send_msg_buff, "KEY");

	if ((send(tmpCli[1].sockfd, tmpCli[1].send_msg_buff, MSG_BUFF_LENGTH, 0)) < 0) {
		printf("\n>> Failed to send private key to CLIENT[1], Error: %d", errno);
		close(tmpCli[0].sockfd);
		close(tmpCli[1].sockfd);
		exit(1);
	}
	printf("\n>> Sent 'KEY' to CLIENT[1]");
	if ((send(tmpCli[1].sockfd, INTPAIR, sizeof(unsigned int) * 2, 0)) < 0) {
		printf("\n Failed to send public key pair to CLIENT[1], Error: %d", errno);
		close(tmpCli[0].sockfd);
		close(tmpCli[1].sockfd);
		exit(1);
	}
	printf("\n>> Sent public key pair to CLIENT[1]");
	/****************************************************
	 follow up message with another message containing 
	 CLIENT[0].protocols, so CLIENT[1] can connect
	****************************************************/
	char tmpHostName[50];
	char tmpAddressName[50];
	getnameinfo((sockaddr*)&tmpCli[0].protocols, sizeof(sockaddr), tmpHostName, 50, tmpAddressName, 50, 0);
	printf("\n\n>> Obtained CLIENT[0] info: \n\tHostname: %s\n\tAddressName: %s", tmpHostName, tmpAddressName);
	char tmpHostName2[50];
	char tmpAddressName2[50];
	getnameinfo((sockaddr*)&tmpCli[1].protocols, sizeof(sockaddr), tmpHostName2, 50, tmpAddressName2, 50, 0);
	printf("\n\n>> Obtained CLIENT[1] info: \n\tHostname: %s\n\tAddressName: %s", tmpHostName2, tmpAddressName2);

	if ((send(tmpCli[1].sockfd, &tmpCli[0].protocols, sizeof(sockaddr), 0)) < 0) {
		printf("\nFailed to send CLIENT[1] protocols for CLIENT[0]");
		close(tmpCli[0].sockfd);
		close(tmpCli[1].sockfd);
		exit(1);
	}
	printf("\n\n>> Sent CLIENT[0] protocols to CLIENT[1]");

	/***********************************************************
	 generate private key and send to CLIENT[0], format: int[2]
	***********************************************************/

	// intPair[0] = e
	// intPair[1] = n

	// THESE INTS ARE NOT THE PROPER VALUES, THIS IS ONLY FOR TESTING
	INTPAIR[0] = 44; INTPAIR[1] = 78;
	if ((send(tmpCli[0].sockfd, INTPAIR, sizeof(int) * 2, 0)) < 0) {
		printf("\n>> Failed to send private key to CLIENT[0], Error: %d", errno);
		close(tmpCli[0].sockfd);
		close(tmpCli[1].sockfd);
		exit(1);
	}
	printf("\n>> Sent private key to CLIENT[0]");
	/*******************************************************
	 follow up message with another message containing
	 CLIENT[0].protocols, so CLIENT[1] can connect
	*******************************************************/
	if ((send(tmpCli[0].sockfd, &tmpCli[1].protocols, sizeof(sockaddr), 0)) < 0) {
		printf("\nMAIN_SERVER failed to send CLIENT[1] protocols for CLIENT[0]");
		close(tmpCli[0].sockfd);
		close(tmpCli[1].sockfd);
		exit(1);
	}
	
	printf("\n>> Closing connection with CLIENT[0]");
	close(tmpCli[0].sockfd);
	printf("\n>> Closing connection with CLIENT[1]");
	close(tmpCli[1].sockfd);
	free(tmpCli);
	printf("\n>> Freeing Client memory");

	//pthread_mutex_unlock(&RECEIVE_MUTEX);
	
	return NULL;
}
void initializeMutexes() {
	pthread_mutex_init(&RECEIVE_MUTEX, NULL);
	pthread_mutex_init(&SENDING_MUTEX, NULL);
	return;
}

int* createPublicKey() {
	/******************************************
	      Public Key algorithm goes here
	******************************************/
}

int* createPrivateKey() {
	/******************************************
	      Private Key algorithm goes here
	******************************************/
}




