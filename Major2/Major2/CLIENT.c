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
	int  encrypted_buff[MSG_BUFF_LENGTH];
	int  int_pair[2];
	unsigned int port;	
	unsigned int d;
	unsigned int e;
	unsigned int n;
} Client;

typedef struct {
	SOCKET sockfd;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	int  int_pair[2];
	unsigned int port;
	unsigned int d;
	unsigned int e;
	unsigned int n;
} Server;

void* receiving();
void* sendMyName();
void* getFriendName();
void  createSocket();
void  setupProtocols(char*);
void  connectSocket();
void  setupAsSever();
void  connectToClientServer();
char  decrypt(int);
void  encrypt(char*);
void  receiveEncryptedMsg();
void  sendEncryptedMsg();

//======================================
//          GLOBAL VARIABLES
//======================================

hostent*        HOST;
pthread_t       RECEIVE_THREAD;
pthread_t       SENDING_THREAD;
pthread_t       SEND_NAME_THREAD;
pthread_t       GET_NAME_THREAD;
pthread_mutex_t MUTEX;
Client          THIS_CLIENT;
Client          THAT_CLIENT;
Client          THIS_CLIENT_SERVER;
Client          THAT_CLIENT_SERVER;
Server          MAIN_SERVER;

//=================================================================
//                            M A I N
//=================================================================
int main(int argc, char* argv[]) {

	pthread_mutex_init(&MUTEX, NULL);
	if (argc < 4) {
		printf("\nUsage: <server hostname> <server port#> <client name>");
		exit(1);
	}
	while (1) {
		MAIN_SERVER.port = atoi(argv[2]);
		strcpy(THIS_CLIENT.name, argv[3]);

		createSocket();
		setupProtocols(argv[1]);
		connectSocket();
		pthread_create(&RECEIVE_THREAD, NULL, &receiving, NULL);
		pthread_join(RECEIVE_THREAD, NULL);
	}
	return 0;
}
//=================================================================

void* sendMyName() {
	send(MAIN_SERVER.sockfd, THIS_CLIENT.name, sizeof(MAIN_SERVER.name), 0);

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
	int check;
	check = connect(MAIN_SERVER.sockfd, (sockaddr*)&MAIN_SERVER.protocols, sizeof(MAIN_SERVER.protocols));
	if (check < 0) {		
		printf("\n>> check = %d", check);
		printf("\n>> Connect failed.. Error: %d\n", errno);
		exit(1);
	}
	printf("\n>> Socket connected\n\n");

	return;
}

void* receiving() {
	int bytesReceived;

	while (1) {		
		memset(THIS_CLIENT.receive_msg_buff, 0, sizeof(THIS_CLIENT.receive_msg_buff));

		if ((bytesReceived = recv(MAIN_SERVER.sockfd, THIS_CLIENT.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0)) < 0) {
			printf("\nTHIS_SERVER failed to get message from MAIN_SERVER\n, Error: %d", errno);
			close(MAIN_SERVER.sockfd);
			exit(1);
		}
		else {
			THIS_CLIENT.receive_msg_buff[bytesReceived] = '\0';
			printf("\n%s", THIS_CLIENT.receive_msg_buff);
		}
		if (THIS_CLIENT.receive_msg_buff == '0') {
			printf("\nReceived '0' from server, closing socket");
			close(THIS_CLIENT.sockfd);
		}
		else if (THIS_CLIENT.receive_msg_buff[0] == 'K' && 
			  THIS_CLIENT.receive_msg_buff[1] == 'E' && 
			   THIS_CLIENT.receive_msg_buff[2] == 'Y') {
			int i;
			int j = 0;
			int k = 0;
			char** received = (char**)malloc(sizeof(char*));
			for (i = 0; i < bytesReceived; ++i) {
				if (THIS_CLIENT.receive_msg_buff[i] == ' ') {
					// expand the array of char* by 1
					++j;
					received = (char**)realloc(received[j], j + 1);
				}
				else {
					// expand the array of char in received[j] by one
					received[j] = (char*)malloc(sizeof(char));
					received[j][k] = THIS_CLIENT.receive_msg_buff[i];
					++k;
				}
			}
			sleep(1);
			connectToClientServer();
		}
		else if (THIS_CLIENT.receive_msg_buff[0] == 'S' && 
			  THIS_CLIENT.receive_msg_buff[1] == 'e' && 
			   THIS_CLIENT.receive_msg_buff[2] == 'n') {
			char* tmp;
			char* tmp2;
			int tmpPair[2];
			printf("\nEnter prime numbers <p q>: ");
			scanf("%d%d", &tmpPair[0], &tmpPair[1]);			

			if ((send(MAIN_SERVER.sockfd, tmpPair, sizeof(int) * 2, 0)) < 0) {
				printf("\nTHIS_CLIENT failed to send intPair to MAIN_SERVER, Error: %d", errno);
				close(MAIN_SERVER.sockfd);
				exit(7);
			}
			printf("\nTHIS_SERVER sent prime numbers to MAIN_SERVER");
			printf("\nSetting up as server");
			setupAsSever();
		}
	}

	return NULL;
}
void setupAsSever() {
	if ((recv(THIS_CLIENT.sockfd, (sockaddr*)&THAT_CLIENT.protocols, sizeof(sockaddr*), 0)) < 0) {
		printf("\nTHIS_CLIENT failed to get protocols for THAT_CLIENT from MAIN_SERVER, Error: %d", errno);
		close(THIS_CLIENT.sockfd);
		exit(8);
	}
	printf("\nObtained protocols for THAT_CLIENT");
	THIS_CLIENT_SERVER.protocols = THIS_CLIENT.protocols;
	//connect on the next port
	++THIS_CLIENT_SERVER.protocols.sin_port;
	if ((THIS_CLIENT_SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {		
		printf("\nCLIENT_SERVER failed to create socket at port %d, ", THIS_CLIENT_SERVER.protocols.sin_port);
		printf("Error: %d", errno);
		exit(2);
	}
	else {
		printf("\nCLIENT_SERVER created socket on port %d", 
			            THIS_CLIENT_SERVER.protocols.sin_port);
	}
	if ((bind(THIS_CLIENT_SERVER.sockfd, (sockaddr*)&THIS_CLIENT_SERVER.protocols, sizeof(THIS_CLIENT_SERVER.protocols))) < 0){		
		printf("\nCLIENT_SERVER failed to bind, Error: %d", errno);		
	}
	printf("\nCLIENT_SERVER bound to port %d", THIS_CLIENT_SERVER.protocols.sin_port);
	if ((listen(THIS_CLIENT_SERVER.sockfd, 1)) < 0) {
		printf("\nTHIS_CLIENT_SERVER failed to listen, Error: %d", errno);
		exit(5);
	}
	printf("\nTHIS_CLIENT_SERVER listening for THAT_CLIENT");
	int sockaddr_size = sizeof(sockaddr_in);
	if ((THAT_CLIENT.sockfd = accept(THIS_CLIENT_SERVER.sockfd, (sockaddr*)&THAT_CLIENT.protocols, &sockaddr_size)) < 0) {
		printf("\nTHIS_CLIENT_SERVER failed to accept THAT_CLIENT, Error: %d", errno);
		exit(6);
	}
	printf("\nTHIS_CLIENT_SERVER accepted THAT_CLIENT");
	sendEncryptedMsg();

	return;
}

void connectToClientServer() {
	int bytesReceived;
	// connecting on the next port
	++THAT_CLIENT_SERVER.protocols.sin_port;
	if ((connect(THAT_CLIENT_SERVER.sockfd, (sockaddr*)&THAT_CLIENT_SERVER.protocols, sizeof(THAT_CLIENT_SERVER.protocols))) < 0) {
		int errorNum = errno;
		printf("\nCould not connect with CLIENT2, Error: %d", errorNum);
		exit(2);
	}
	printf("\nTHIS_CLIENT connected with THAT_CLIENT_SERVER");
	receiveEncryptedMsg();
}


char decrypt(int tmp) {
	char letter;
	/******************************************
	      Decryption algorithm goes here
	 ******************************************/
	return letter;
}

void encrypt(char* msg) {
	/******************************************
	      Encryption algorithm goes here
	 ******************************************/
	return;
}

void receiveEncryptedMsg() {
	int bytesReceived;
	while (1) {
		memset(THIS_CLIENT.encrypted_buff, 0, MSG_BUFF_LENGTH * sizeof(int));
		if (bytesReceived = (recv(THAT_CLIENT_SERVER.sockfd, THIS_CLIENT.encrypted_buff, MSG_BUFF_LENGTH * sizeof(int), 0)) < 0) {
			printf("\nTHIS_CLIENT failed to receive message from THAT_CLIENT_SERVER, Error: %d", errno);
			printf("\nExiting...");
			exit(3);
		}
		printf("\nReceived encrypted message from THAT_CLIENT_SERVER");
		int i;
		for (i = 0; i < (bytesReceived / 4); ++i) {
			printf("%c", decrypt(THIS_CLIENT.encrypted_buff[i]));
		}
	}
}

void sendEncryptedMsg() {
	char* msg;
	while (1) {
		printf("\nMessage: ");
		scanf("%s", msg);
		memset(THIS_CLIENT_SERVER.encrypted_buff, 0, MSG_BUFF_LENGTH * sizeof(int));
		int i;
		for (i = 0; i < strlen(msg); ++i) {
			// THIS_CLIENT_SERVER.encrypted_buff[i] = encrypt(msg[i]);
		}
		if ((send(THAT_CLIENT.sockfd, THIS_CLIENT_SERVER.encrypted_buff, MSG_BUFF_LENGTH * sizeof(int), 0)) < 0) {
			printf("\nTHIS_CLIENT_SERVER failed to send encrypted message to THAT_CLIENT, Error: %d", errno);
		}
		else {
			printf("\nEncrypted message sent to THAT_CLIENT");
		}
	}
}
