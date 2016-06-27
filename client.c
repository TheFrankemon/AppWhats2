#include <stdio.h>
#include <string.h>   //strlen
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>   //close
#include <arpa/inet.h>    //close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h> //FD_SET, FD_ISSET, FD_ZERO macros

#define TRUE   1
#define FALSE  0
#define PORT 8888
#define STDIN 0

/*typedef struct {
  char nick[50];
  struct sockaddr_in sockAddr;
  struct client * next;
} client;*/

/*struct Client
{
	char nick[50];
	struct sockaddr_in sockAddr;
};*/

void validateArgs(int argc, char *argv[]) {
	if (argc != 4) {
		perror("error: must give 3 params (server_ip server_port nick)");
		exit(EXIT_FAILURE);
	}
}

int main(int argc , char *argv[])
{
	validateArgs(argc, argv);
	
	char *serverIP = argv[1];
	
	char *err;
	int serverPort = strtol(argv[2], &err, 10); // converts port string to int
    if (err[0] != '\0') { // bad input, not a number
    	perror("error: char string (invalid port)");
    	exit(EXIT_FAILURE);
    }

	int socketFD = socket(AF_INET, SOCK_STREAM, 0);// IPPROTO_TCP);
	if (-1 == socketFD) {
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	//bind the socket to localhost port 8888
	/*if (bind(socketFD, (struct sockaddr *)&stSockAddr, sizeof(stSockAddr))<0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}*/
	//printf("Listener on port %d \n", PORT);

	int res;
	struct sockaddr_in stSockAddr;
	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(serverPort);
	res = inet_pton(AF_INET, serverIP, &stSockAddr.sin_addr);

	if (0 > res) {
		perror("error: first parameter is not a valid address family");
		close(socketFD);
		exit(EXIT_FAILURE);
	}
	else if (0 == res) {
		perror("error: char string (invalid ip address)");
		close(socketFD);
		exit(EXIT_FAILURE);
	}

	if (-1 == connect(socketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in))) {
		perror("error: connect failed");
		close(socketFD);
		exit(EXIT_FAILURE);
	}

	printf("Connecting to %s:%d\n", serverIP, serverPort);

	send(socketFD, argv[3] , strlen(argv[3]) , 0);

	/*struct client cData;
	strcpy(cData.nick, argv[3]);
	cData.sockAddr = stSockAddr;
	cData.next = NULL;*/

	int activity, valread;
	int max_sd;

	char buffer[1025];  //data buffer of 1K
	
	//set of socket descriptors
	fd_set readfds;

	//calculates max file descriptor
	max_sd = (socketFD > STDIN) ? socketFD : STDIN;
	
	while(1) 
	{
		//clear the socket set
		FD_ZERO(&readfds);
		
		//add server socket to set
		FD_SET(socketFD, &readfds);
		FD_SET(STDIN, &readfds);

		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) {
			printf("select error");
		}
		
		//Server activity
		if (FD_ISSET(socketFD, &readfds)) {
			valread = read(socketFD, buffer, 1024);
			buffer[valread] = '\0';
			//printf("Msg: %s\n----------------------\n", buffer);
			if (valread == 0) {
				exit(EXIT_SUCCESS);
			} else {
				printf("%s\n", buffer);
			}
		}
		
		//STDIN activity
		if (FD_ISSET(STDIN, &readfds)) {
			valread = read(STDIN, buffer, 1024);
			buffer[valread] = '\0';
			send(socketFD, buffer , strlen(buffer) , 0);

			/*if (strncmp(buffer, "#quit", 5) == 0) {
				exit(EXIT_SUCCESS);
			}*/
		}
	}

	exit(EXIT_FAILURE);
}