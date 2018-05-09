#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
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
	unsigned int encrypted_buff[MSG_BUFF_LENGTH];
	unsigned int primes[2];
	unsigned int port;
	unsigned int d;
	unsigned int e;
	unsigned int n;
} Client;

typedef struct {
	SOCKET sockfd;
	SOCKET sockfd_msg;
	sockaddr_in protocols;
	char name[20];
	char send_msg_buff[MSG_BUFF_LENGTH];
	char receive_msg_buff[MSG_BUFF_LENGTH];
	unsigned int primes[2];
	unsigned int port;
	unsigned int d;
	unsigned int e;
	unsigned int n;
} Server;

void* receiving();
void  createSocket();
void  setupProtocols(char*);
void  connectSocket();
void  setupAsSever();
void  connectToClientServer();
char  decrypt(int);
void  encrypt(char*);
void  receiveEncryptedMsg();
void  sendEncryptedMsg();
void* MessagesFromServer();

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

	system("clear");
	pthread_mutex_init(&MUTEX, NULL);

	printf("\n=========================================");
	printf("\n             %c %c %c %c %c %c  %c", toupper(argv[3][0]), toupper(argv[3][1]),
		toupper(argv[3][2]), toupper(argv[3][3]),
		toupper(argv[3][4]), toupper(argv[3][5]),
		toupper(argv[3][6]));
	printf("\n=========================================\n");

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
	if ((connect(MAIN_SERVER.sockfd, (sockaddr*)&MAIN_SERVER.protocols, sizeof(sockaddr))) < 0) {
		printf("\n>> Could not connect with MAIN_SERVER, Error: %d", errno);
		exit(1);
	}
	printf("\n>> Socket connected\n\n");
}

void* receiving() {
	int bytesReceived;

	while (1) {
		memset(THIS_CLIENT.receive_msg_buff, NULL, MSG_BUFF_LENGTH);

		if ((bytesReceived = recv(MAIN_SERVER.sockfd, THIS_CLIENT.receive_msg_buff, MSG_BUFF_LENGTH - 1, 0)) < 0) {
			printf("\n>> THIS_SERVER failed to get message from MAIN_SERVER\n, Error: %d", errno);
			close(MAIN_SERVER.sockfd);
			exit(1);
		}
		THIS_CLIENT.receive_msg_buff[strlen(THIS_CLIENT.receive_msg_buff)] = '\0';
		printf("\n>> MAIN_SERVER: %s", THIS_CLIENT.receive_msg_buff);

		if ((THIS_CLIENT.receive_msg_buff[0] == 'S') &&
			(THIS_CLIENT.receive_msg_buff[1] == 'e') &&
			(THIS_CLIENT.receive_msg_buff[2] == 'n')) {

			do {
				memset(THIS_CLIENT.primes, NULL, sizeof(unsigned int) * 2);
				printf("\n\nEnter prime numbers <p q>: ");
				scanf("%d%d", &THIS_CLIENT.primes[0], &THIS_CLIENT.primes[1]);

				if ((send(MAIN_SERVER.sockfd, THIS_CLIENT.primes, sizeof(unsigned int) * 2, 0)) < 0) {
					printf("\n>> THIS_CLIENT failed to send intPair to MAIN_SERVER, Error: %d", errno);
					close(MAIN_SERVER.sockfd);
					exit(7);
				}
				printf("\n>> Sent prime numbers to MAIN_SERVER");
				memset(THIS_CLIENT.receive_msg_buff, NULL, MSG_BUFF_LENGTH);
				recv(MAIN_SERVER.sockfd, THIS_CLIENT.receive_msg_buff, MSG_BUFF_LENGTH, 0);
				if (memcmp(THIS_CLIENT.receive_msg_buff, "Accepted", strlen("Accepted"))) {
					printf("\n>> MAIN_SERVER: Primes accepted");
					printf("\n>> Setting up as server");
					setupAsSever();
					return NULL;
				}

			} while (memcmp(THIS_CLIENT.receive_msg_buff, "Denied", strlen("Denied")));
		}
		else if (THIS_CLIENT.receive_msg_buff == "0") {
			printf("\nReceived '0' from server, closing socket");
			close(MAIN_SERVER.sockfd);
			exit(1);
		}
		else if (memcmp(THIS_CLIENT.receive_msg_buff, "KEY", 3)) {
			memset(THIS_CLIENT.primes, NULL, sizeof(unsigned int) * 2);
			if ((recv(MAIN_SERVER.sockfd, (unsigned int*)THIS_CLIENT.primes, sizeof(unsigned int) * 2, 0)) < 0) {
				printf("\n>> Did not receive public key from MAIN_SERVER, Error: %d", errno);
				close(MAIN_SERVER.sockfd);
				exit(1);
			}
			printf("\n>> Received public key from MAIN_SERVER: %d, %d", THIS_CLIENT.primes[0], THIS_CLIENT.primes[1]);

			memset(&THAT_CLIENT_SERVER.protocols, NULL, sizeof(sockaddr));
			if ((recv(MAIN_SERVER.sockfd, (sockaddr_in*)&THAT_CLIENT_SERVER.protocols, sizeof(sockaddr), 0)) < 0) {
				printf("\n>> Did not receive THAT_CLIENT_SERVER protocols from MAIN_SERVER, Error: %d", errno);
				close(MAIN_SERVER.sockfd);
				exit(1);
			}
			printf("\n>> Received protocols for THAT_CLIENT_SERVER from MAIN_SERVER");
			connectToClientServer();
			return NULL;
		}
	}

	return NULL;
}
void setupAsSever() {
	if ((recv(MAIN_SERVER.sockfd, (sockaddr_in*)&THAT_CLIENT.protocols, sizeof(sockaddr), 0)) < 0) {
		printf("\n>> THIS_CLIENT failed to get protocols for THAT_CLIENT from MAIN_SERVER, Error: %d", errno);
		close(THIS_CLIENT.sockfd);
		exit(8);
	}
	memcpy(&THIS_CLIENT_SERVER.protocols, &MAIN_SERVER.protocols, sizeof(sockaddr));

	printf("\n>> Obtained protocols for THAT_CLIENT");
	close(MAIN_SERVER.sockfd);
	printf("\n>> Disconnected from MAIN_SERVER...");
	printf("\n>> Closed original socket");


	//connect on the next port
	THIS_CLIENT_SERVER.protocols.sin_port = htons(THIS_CLIENT_SERVER.protocols.sin_port + 2);
	if ((THIS_CLIENT_SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n>> CLIENT_SERVER failed to create socket at port %d\n", THIS_CLIENT_SERVER.protocols.sin_port);
		printf("Error: %d", errno);
		exit(2);
	}
	printf("\n>> Created socket on port %d", THIS_CLIENT_SERVER.protocols.sin_port);

	if ((bind(THIS_CLIENT_SERVER.sockfd, (sockaddr*)&THIS_CLIENT_SERVER.protocols, sizeof(sockaddr))) < 0) {
		printf("\n>> Failed to bind, Error: %d\n", errno);
		exit(1);
	}
	printf("\n>> Bound to port %d", THIS_CLIENT_SERVER.protocols.sin_port);
	if ((listen(THIS_CLIENT_SERVER.sockfd, 1)) < 0) {
		printf("\n>> Failed to listen, Error: %d\n", errno);
		exit(5);
	}
	printf("\n>> Listening for THAT_CLIENT");
	socklen_t sockaddr_size = sizeof(sockaddr_in);
	if ((THAT_CLIENT.sockfd = accept(THIS_CLIENT_SERVER.sockfd, (sockaddr*)&THAT_CLIENT.protocols, &sockaddr_size)) < 0) {
		printf("\n>> Failed to accept THAT_CLIENT, Error: %d\n", errno);
		exit(6);
	}
	printf("\n>> Established connection to THAT_CLIENT");
	sendEncryptedMsg();

	return;
}

void connectToClientServer() {
	int bytesReceived;
	// connecting on the next port	
	memcpy(&THAT_CLIENT_SERVER.protocols, &MAIN_SERVER.protocols, sizeof(sockaddr));
	close(MAIN_SERVER.sockfd);
	printf("\n>> Disconnected from MAIN_SERVER");
	THAT_CLIENT_SERVER.protocols.sin_port = htons(THAT_CLIENT_SERVER.protocols.sin_port + 2);
	memset(&THIS_CLIENT, 0, sizeof(Client));
	if ((THAT_CLIENT_SERVER.sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		printf("\n>> Failed to create new socket");
	}
	printf("\n>> New socket created at port %d", THAT_CLIENT_SERVER.protocols.sin_port);

	if ((connect(THAT_CLIENT_SERVER.sockfd, (sockaddr*)&THAT_CLIENT_SERVER.protocols, sizeof(sockaddr))) < 0) {
		int errorNum = errno;
		printf("\n>> Could not connect with THAT_CLIENT_SERVER, Error: %d", errorNum);
		exit(2);
	}
	printf("\n>> Connected with THAT_CLIENT_SERVER");
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
	int msgSize;
	printf("\n\n\t *****NO DECRYPTION FUNCTION YET. HELP!******");
	//printf("\n\n>> Exiting just before receiveEncryptedMsg() runs loop\n\n\n");
	//exit(1);
	printf("\n\n>> For testing...");
	while (1) {
		memset(THIS_CLIENT_SERVER.receive_msg_buff, 0, MSG_BUFF_LENGTH);

		recv(THAT_CLIENT_SERVER.sockfd, (int*)&msgSize, sizeof(int), 0);
		printf("\n>> Incoming message size: %d", msgSize);
		char eMsg[msgSize];
		if ((bytesReceived = recv(THAT_CLIENT_SERVER.sockfd, eMsg, msgSize, 0)) < 0) {
			printf("\n>> Failed to receive message from THAT_CLIENT_SERVER, Error: %d", errno);
			printf("\n>> Exiting...");
			exit(3);
		}
		printf("\n>> Size of eMsg after recv: %d", sizeof(*eMsg));
		printf("\n>> Bytes received: %d", bytesReceived);
		printf("\n\n>> Received encrypted message from THAT_CLIENT_SERVER");
		printf("\n>> THAT_CLIENT_SERVER: ");
		int i;
		for (i = 0; i < msgSize; ++i) {
			printf("%d", (int)eMsg[i]);
			printf("[%c] ", eMsg[i]);
			//printf("%c", decrypt(THIS_CLIENT.encrypted_buff[i]));
		}
		memset(eMsg, 0, msgSize * sizeof(int));
		memset(&msgSize, 0, sizeof(int)*msgSize);
	}
}

void sendEncryptedMsg() {
	char msg[50];
	int size;
	int i;
	printf("\n\n\t *****NO ENCRYPTION FUNCTION YET. HELP!*****");
	printf("\n\n>> For testing...");

	while (1) {
		printf("\n\n>> Message: ");
		do {
			scanf(" %s", &msg[0]);
		} while ((strlen(msg) <= 0));

		size = strlen(msg);
		printf("\n>> Message: %s", msg);

		char eMsg[size];
		printf("\n>> sizeof(int)*size: %d\n", sizeof(int)*size);
		for (i = 0; i < strlen(msg); ++i) {
			eMsg[i] = msg[i];
			//THIS_CLIENT_SERVER.encrypted_buff[i] = encrypt(msg[i]);
		}
		for (i = 0; i < size; ++i) {
			printf("%d", *(eMsg + i));
			printf("[%c] ", (char)eMsg[i]);
			//printf("%c", decrypt(THIS_CLIENT.encrypted_buff[i]));
		}
		printf("\n>> Size of eMsg: %d", sizeof(eMsg));
		printf("\n>> Size of message: %d", size);
		send(THAT_CLIENT.sockfd, &size, sizeof(int), 0);
		if ((send(THAT_CLIENT.sockfd, eMsg, size, 0)) < 0) {
			printf("\n>> Failed to send encrypted message to THAT_CLIENT, Error: %d", errno);
		}
		else {
			printf("\n>> Encrypted message sent to THAT_CLIENT");
		}
		memset(eMsg, 0, size * sizeof(int));
		memset(msg, 0, 50);
	}
}

void * MessagesFromServer() {
	while (1) {

	}
	return NULL;
}
