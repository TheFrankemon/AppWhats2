#include <stdio.h>
#include <string.h>   //strlen, strncmp, strcpy
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

/**
 * Struct that defines a client, it also can be used as a client list
 */
typedef struct {
  char nick[50]; // nickname
  int fd; // file descriptor
  struct client * next; // used for dynamic list purposes
  int hasNick; // used for client login
} client;

/**
 * @brief      Removes a client from a given list pointer.
 *
 * @param      list  The list
 * @param[in]  fd    The fd of the client to be removed
 *
 * @return     1 if client is removed successfully
 */
int removeClient(client** list, int fd) {
    client* tmp = *list;

    if (tmp == NULL) { // null list
        return 0;
    }

    if (tmp -> fd == fd) { // client to be removed is the first
        *list = (client *) tmp -> next;
        return 1;
    } else {
        for (tmp = *list; tmp -> next != NULL; ) {
            if (((client*)(tmp -> next)) -> fd == fd) { // client has been found
                tmp -> next = ((client *)(tmp -> next)) -> next;
                return 1;
            }
            tmp = (client*)(tmp->next);
        }
    }
    return 0;
}

/**
 * @brief      Inserts a client to the end of a given list pointer.
 *
 * @param      list  The list
 * @param      elem  The element to be inserted
 *
 * @return     1 if elements is inserted successfully
 */
int pushClient(client** list, client** elem) {
    client* tmp = NULL;
  
    if (*elem == NULL) { // Element to insert is null
        return 0; 
    }
  
    if (*list == NULL) { // List is empty
        *list = *elem;
        *elem = NULL;
        return 1;
    }

    for (tmp = *list; tmp->next != NULL; ) { // Gets to the end of the list
        tmp = (client*)(tmp->next);
    }
    tmp->next = (struct client*) *elem; // Insert element to the end
    *elem = NULL;
  
    return 1;
}

/**
 * @brief      Closes client's socket and removes it from list
 *
 * @param      connectedClients  The connected clients list
 * @param[in]  sd                Client to the disconnected
 */
void disconnectClient(client** connectedClients, int sd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    getpeername(sd , (struct sockaddr*) &address , (socklen_t*) &addrlen); // gets socket info
    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
    
    close(sd);
    removeClient(connectedClients, sd);
}

/**
 * @brief      Checks if a nickname is already being used.
 *
 * @param      list       The client list
 * @param      newNick    The new nick
 * @param      buffer     The buffer to save the error message
 *
 * @return     1 is nickname is not being used.
 */
int checkNickname(client* list, char* newNick, char* buffer) {
    client* tmp = list;
    for (; tmp != NULL; ) {
        if (tmp -> hasNick && strcmp(tmp -> nick, newNick) == 0) { // Nickname is already being used
            sprintf(buffer, "Sorry '%s' is already connected, please change nickname and retry...", newNick);
            return 0;
        }
        tmp = (client*)(tmp->next);
    }

    if (strstr(newNick, " ") != NULL) { // Nickname contain spaces
        sprintf(buffer, "Nickname can't contain spaces");
        return 0;
    }

    return 1;
}

/**
 * @brief      Sends a message to a given file descriptor socket.
 *
 * @param[in]  fd      Socket for the message to be sent
 * @param      buffer  The message to be sent
 *
 * @return     1 if message is successfully sent
 */
int sendMessageTo(int fd, char* buffer) {
    if(send(fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
        perror("send");
        return 0;
    }

    return 1;
}

/**
 * @brief      Sends a message to all the clients except a given fd
 *
 * @param      list    The list of clients
 * @param      buffer  The message
 * @param[in]  fd      fd that won't receive the message
 *
 * @return     1 if the message is successfully sent
 */
int sendMessageToAllExcept(client* list, char* buffer, int fd) {
    client* tmp = list;

    if (tmp == NULL) {
        return 0;
    }

    for (; tmp != NULL; ) {
        if (tmp -> fd != fd) {
            sendMessageTo(tmp -> fd, buffer);
        }
        tmp = (client*)(tmp->next);
    }

    return 1;
}

/**
 * @brief      Sends a message to all the clients
 *
 * @param      list    The list of clients
 * @param      buffer  The message
 *
 * @return     1 if the message is successfully sent
 */
int sendMessageToAll(client* list, char* buffer) {
    client* tmp = list;

    if (tmp == NULL) {
        return 0;
    }

    for (; tmp != NULL; ) {
        sendMessageTo(tmp -> fd, buffer);
        tmp = (client*)(tmp->next);
    }

    return 1;
}

/**
 * @brief      Gets all clients and saves them in the buffer.
 *
 * @param      list    The list of clients
 * @param      buffer  The buffer to save the message
 *
 * @return     0 if the list is empty
 */
int getAllClients(client* list, char* buffer) {
    char aux[1025];
    client* tmp = list;
    strcpy(buffer, "> Connected users:");

    if (tmp == NULL) {
        return 0;
    } else { // First client
        sprintf(aux, "%s %s", buffer, tmp -> nick);
        strcpy(buffer, aux);
        tmp = (client*)(tmp->next);
    }

    while (tmp != NULL) { // The rest of the clients separated by a comma
        sprintf(aux, "%s, %s", buffer, tmp -> nick);
        strcpy(buffer, aux);
        tmp = (client*)(tmp->next);
    }

    return 1;
}

/**
 * @brief      Sends a private message.
 *
 * @param      from    The sender
 * @param      buffer  The message
 * @param      list    The list clients
 *
 * @return     1 if message is successfully sent
 */
int sendPrivateMessage(client* from, char* buffer, client* list) {
    client* tmp = list;

    char *newline = strchr(buffer, '\n'); // Removes new line characters, to split the message correctly
    if (newline) {
      *newline = 0;
    }

    char *nickTo, aux[1025], msg[1025];
    strcpy(aux, buffer);

    nickTo = strtok(buffer + 1, " ");
    strcpy(msg, aux + 1 + strlen(nickTo) + 1);
    sprintf(aux, "#private %s> %s", from -> nick, msg);
    
    if (strcmp(from -> nick, nickTo) == 0) { // Tried to send a private message to the same user
        sendMessageTo(from -> fd, "> Can't send private messages to yourself");
        return 0;
    }

    while (tmp != NULL) {
        if (strcmp(tmp -> nick, nickTo) == 0) { // Client is found
            sendMessageTo(tmp -> fd, aux);
            return 1;
        }
        tmp = (client*)(tmp->next);
    }

    sprintf(aux, "> %s is not connected", nickTo);
    sendMessageTo(from -> fd, aux);
    return 0; // Client is not found
}

/**
 * @brief      Validates the port given as a parameter
 *
 * @param[in]  argc  Number of parameters
 * @param      argv  Array of strings
 *
 * @return     the port
 */
int validatePort(int argc, char *argv[]) {
    int port;
    char *err;

    if (argc != 2) { // invalid number of arguments
        printf("Must specify the port where the server will listen.\n");
        exit(EXIT_FAILURE);
    }

    port = strtol(argv[1], &err, 10);
    if (err[0] != '\0') { // bad input, not a number
        printf("Invalid port number\n");
        exit(EXIT_FAILURE);
    }

    return port;
}

/**
 * @brief      Initialiazes the server socket
 *
 * @param[in]  port  The port the server will listen on
 *
 * @return     The socket file descriptor
 */
int initializeMasterSocket(int port) {
    int master_socket;
    int opt = TRUE;
    struct sockaddr_in address;

    // creates a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // sets master socket to allow multiple connections
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( port );
      
    //bind the socket to a defined port
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listening on port %d \n", port);

    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    puts("AppWhats2 chat server started... Waiting for connections...");

    return master_socket;
}

/**
 * @brief      Clears the set of socket descriptors and adds the ones we will listen to
 *
 * @param[in]  master_socket     The master socket
 * @param      readfds           The readfds set of socket descriptors
 * @param      connectedClients  The connected clients list
 *
 * @return     the max file descriptor
 */
int cleanSocketFDSet(int master_socket, fd_set* readfds, client* connectedClients) {
    int sd, max_fd;

    //clear the socket set
    FD_ZERO(readfds);

    //add master socket to set
    FD_SET(master_socket, readfds);
    max_fd = master_socket;
     
    //add child sockets to set
    for (client* tmp = connectedClients; tmp != NULL; tmp = (client*) tmp -> next) {
        sd = tmp -> fd;
        if(sd > 0) {
            FD_SET(sd, readfds);
        }
        if(sd > max_fd) {
            max_fd = sd;
        }
    }

    return max_fd;
}

/**
 * @brief      Connects a new client.
 *
 * @param[in]  master_socket  The server socket fd
 * @param      list        The list of clients
 *
 * @return     1 if client is connected successfully
 */
int connectClient(int master_socket, client** list) {
    int new_socket;
    int addrlen;
    struct sockaddr_in address;

    if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
  
    printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));

    client* newClient = (client*) malloc(sizeof(client));
    newClient -> fd = new_socket;
    newClient -> next = NULL;
    newClient -> hasNick = 0;
    return pushClient(list, &newClient);
}

/**
 * @brief      Tries to login the new client with a given nickname
 *
 * @param      tmp     The new client
 * @param      list    The list of clients
 * @param      buffer  The nickname
 */
void loginClient(client* tmp, client* list, char* buffer) {
    char newNick[50];
    strcpy(newNick, buffer);
    if (checkNickname(list, newNick, buffer)) { // valid nickname
        strcpy(tmp -> nick, newNick);
        tmp -> hasNick = 1;

        printf("%s has connected\n", tmp -> nick);
        sprintf(buffer, "> @%s has joined the chat", tmp -> nick);

        sendMessageTo(tmp -> fd, "Welcome!");
        sendMessageToAllExcept(list, buffer, tmp -> fd);
    } else { // invalid nickname
        sendMessageTo(tmp -> fd, buffer);
    }
}

/**
 * @brief      Handles a given client's activity
 *
 * @param      tmp               The client with activity
 * @param      connectedClients  The connected clients list
 */
void handleClientActivity(client* tmp, client* connectedClients) {
    int valread, sd = tmp -> fd;
    char buffer[1025], auxBuffer[1025];

    if ((valread = read(sd, buffer, 1024)) == 0) { // Client has disconnected
        disconnectClient(&connectedClients, sd);
        sprintf(buffer, "> @%s lost connection", tmp -> nick);
        sendMessageToAll(connectedClients, buffer);
    } else { // Client sent a message
        buffer[valread] = '\0';
        if (!(tmp -> hasNick)) { // Client is trying to login
            loginClient(tmp, connectedClients, buffer);
        } else { // Client has already logged in
            if (strncmp(buffer, "#quit", 5) == 0) { // Client requested to disconnect
                disconnectClient(&connectedClients, sd);
                sprintf(buffer, "> @%s has left the chat", tmp -> nick);
                sendMessageToAll(connectedClients, buffer);
            } else if (strncmp(buffer, "#showall", 8) == 0) { // Client requested to see all the users
                getAllClients(connectedClients, buffer);
                sendMessageTo(sd, buffer);
            } else if (strncmp(buffer, "@", 1) == 0) { // Private message
                sendPrivateMessage(tmp, buffer, connectedClients);
            } else { // Normal message
                sprintf(auxBuffer, "%s> %s", tmp -> nick, buffer);
                sendMessageToAll(connectedClients, auxBuffer);
            }
        }
    }
}
 
int main(int argc , char *argv[]) {
    int port; // port the server will listen on
    int master_socket; // server socket
    int activity; // used for select
    int max_fd; // max file descriptor

    client* connectedClients = NULL; // list of clients
    client* tmp; // auxiliary client to iterate on the list

    fd_set readfds; // set of socket descriptors
    
    port = validatePort(argc, argv);
    master_socket = initializeMasterSocket(port);
     
    while(TRUE) {
        max_fd = cleanSocketFDSet(master_socket, &readfds, connectedClients);
  
        // Wait for an activity on one of the sockets
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
    
        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }
          
        // Incoming client connection
        if (FD_ISSET(master_socket, &readfds)) {
            connectClient(master_socket, &connectedClients);
        }
          
        // IO operation on some of the client's sockets
        for (tmp = connectedClients; tmp != NULL;) {
            if (FD_ISSET(tmp -> fd, &readfds)) { // Found the client that has activity
                handleClientActivity(tmp, connectedClients);
            }
            tmp = (client*) tmp -> next;
        }
    }
    return 0;
}