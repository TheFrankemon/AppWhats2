/**
    Handle multiple socket connections with select and fd_set on Linux   
    Silver Moon ( m00n.silv3r@gmail.com)
*/
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

typedef struct {
  char nick[50];
  //struct sockaddr_in sockAddr;
  int socketFD;
  struct client * next;
  int firstMsg;
} client;

int removeClient(client** list, int fd) {
    client* tmp = *list;
    if (tmp == NULL) {
        return 0;
    }
    if (tmp -> socketFD == fd) {
        //tmp = *list;
        *list = (client *) tmp -> next;
        return 1;
    } else {
        for (tmp = *list; tmp -> next != NULL; ) {
            //tmp = (client*)(tmp->next);
            if (((client*)(tmp -> next)) -> socketFD == fd) {
                tmp -> next = (client *)(((client *)(tmp -> next)) -> next);
                return 1;
            }
            /*if (tmp -> socketFD == fd) {
                tmp = (client *) tmp -> next;
                return 1;
            }*/
            tmp = (client*)(tmp->next);
        }
    }

    return 0;
}

int pushClient(client** list, client** elem) {
    client* tmp = NULL;
  
    if (*elem == NULL) {
        //printf ("El elemento a insertar es nulo. ");
        return 0; 
    }
  
    if (*list == NULL) {
        //printf ("Se inserta el primer elemento.\n");
        *list = *elem;
        *elem = NULL;
        //mostrar_punto(**cadena);
    }
    else {
        //printf ("La cadena no esta vacia\n");
        for (tmp = *list; tmp->next != NULL; ) {
            tmp = (client*)(tmp->next);
        }
        tmp->next = (struct client*) *elem;
        *elem = NULL;
    }
  
    return 1;
}

void disconnectClient(client** connectedClients, int sd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    getpeername(sd , (struct sockaddr*) &address , (socklen_t*) &addrlen);
    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
      
    //Close the socket and mark as 0 in list for reuse
    close(sd);
    //client_socket[i] = 0;
    removeClient(connectedClients, sd);
}

int checkNickname(client* list, client* newClient) {
    client* tmp = list;
    for (; tmp != NULL; ) {
        if (tmp -> socketFD != newClient -> socketFD &&
            strcmp(tmp -> nick, newClient -> nick) == 0) {
            return 0;
        }
        tmp = (client*)(tmp->next);
    }
    return 1;
}

int sendMessageTo(int fd, char* buffer) {
    if(send(fd, buffer, strlen(buffer), 0) != strlen(buffer)) {
        perror("send");
    }

    return 1;
}

int sendMessageToAllExcept(client* list, char* buffer, int fd) {
    client* tmp = list;

    if (tmp == NULL) {
        return 0;
    }

    for (; tmp != NULL; ) {
        if (tmp -> socketFD != fd) {
            sendMessageTo(tmp -> socketFD, buffer);
        }
        tmp = (client*)(tmp->next);
    }

    return 1;
}

int sendMessageToAll(client* list, char* buffer) {
    client* tmp = list;

    if (tmp == NULL) {
        return 0;
    }

    for (; tmp != NULL; ) {
        sendMessageTo(tmp -> socketFD, buffer);
        tmp = (client*)(tmp->next);
    }

    return 1;
}

int getAllClients(client* list, char* buffer) {
    char aux[1025];
    client* tmp = list;
    strcpy(buffer, "> Connected users:");
    if (tmp == NULL) {
        return 0;
    } else {
        sprintf(aux, "%s %s", buffer, tmp -> nick);
        strcpy(buffer, aux);
        tmp = (client*)(tmp->next);
    }
    //aux[strlen(aux)] = '\0';

    while (tmp != NULL) {
        sprintf(aux, "%s, %s", buffer, tmp -> nick);
        strcpy(buffer, aux);
        tmp = (client*)(tmp->next);
    }

    return 1;
}

int sendCommonMessage(client* from, char* buffer, client* list) {
    client* tmp = list;
    char aux[strlen(buffer) + strlen(from -> nick) + 3 + 1];

    sprintf(aux, "%s > %s", from -> nick, buffer);
    aux[strlen(buffer) + strlen(from -> nick) + 3] = '\0';
    //strcpy(buffer, aux);
    //buffer[strlen(aux)] = '\0';
    //printf("%d\n", strlen(aux));

    sendMessageToAll(list, aux);

    /*client* tmp = list;
    char aux[1025], temp[75];
    int curPos = 0;

    strcpy(aux, buffer);

    printf("%d\n", strlen(aux) - curPos);

    while (strlen(aux) - curPos > 74) {
        memcpy(temp, aux + curPos, 74);
        temp[74] = '\0';
        sprintf(buffer, "%s > %s\n", from -> nick, temp);
        sendMessageToAll(list, buffer);        
        curPos += 74;
    }

    sprintf(buffer, "%s > %s", from -> nick, aux + curPos);
    sendMessageToAll(list, buffer);*/
    
    return 1;
}

int sendPrivateMessage(client* from, char* buffer, client* list) {
    client* tmp = list;
    /*int nickLength = index(buffer, ' ') - 1;
    char nickTo[nickLength], aux[1025];
    printf("%s : %d\n", buffer, nickLength);
    strncpy(nickTo, buffer + 1, nickLength);
    printf("%s\n", nickTo);
    //nickTo[nickLength] = '\0';
    strcpy(aux, buffer + 2 + nickLength);
    printf("%s -> %s\n", nickTo, aux);
    sprintf(buffer, "#private %s> %s", from -> nick, aux);*/

    char *newline = strchr(buffer, '\n');
    if (newline) {
      *newline = 0;
    }

    char *nickTo, aux[1025], msg[1025];
    strcpy(aux, buffer);

    nickTo = strtok(buffer + 1, " ");
    strcpy(msg, aux + 1 + strlen(nickTo) + 1);
    sprintf(aux, "#private %s> %s", from -> nick, msg);

    //sprintf(msg, "'%s' -> '%s'\n", nickTo, buffer);
    //printf("'%s' -> '%s'", nickTo, msg);
    //printf("ooook\n");
    
    if (strcmp(from -> nick, nickTo) == 0) {
        sendMessageTo(from -> socketFD, "> Can't send private messages to yourself");
        return 0;
    }

    while (tmp != NULL) {
        if (strcmp(tmp -> nick, nickTo) == 0) {
            sendMessageTo(tmp -> socketFD, aux);
            return 1;
        }
        tmp = (client*)(tmp->next);
    }

    sprintf(aux, "> %s is not connected", nickTo);
    sendMessageTo(from -> socketFD, aux);
    return 0;
}
 
int main(int argc , char *argv[]) {
    //Verify if the server has its defined Port.
    if (argc != 2) {
        printf("Must specify the port where the server will listen.\n");
        exit(-1);
    }

    int PORT = atoi(argv[1]);
    int opt = TRUE;
    int master_socket , addrlen , new_socket , client_socket[30] , max_clients = 30 , activity, i , valread , sd;
    int max_sd;
    struct sockaddr_in address;

    client* connectedClients = NULL;
    client* tmp;
      
    char buffer[1025];  //data buffer of 1K
    char auxBuffer[1025];

    auxBuffer[0] = '\0';

    //set of socket descriptors
    fd_set readfds;
  
    //initialise all client_socket[] to 0 so not checked
    /*for (i = 0; i < max_clients; i++) {
        client_socket[i] = 0;
    }*/
      
    //create a master socket
    if( (master_socket = socket(AF_INET , SOCK_STREAM , 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
  
    //set master socket to allow multiple connections , this is just a good habit, it will work without this
    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0 ) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
  
    //type of socket created
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
      
    //bind the socket to a defined port
    if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    printf("Listener on port %d \n", PORT);
     
    //try to specify maximum of 3 pending connections for the master socket
    if (listen(master_socket, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
      
    //accept the incoming connection
    addrlen = sizeof(address);
    puts("AppWhats2 chat server started... Waiting for connections...");
     
    while(TRUE) {
        //clear the socket set
        FD_ZERO(&readfds);
  
        //add master socket to set
        FD_SET(master_socket, &readfds);
        max_sd = master_socket;
         
        //add child sockets to set
        for (tmp = connectedClients; tmp != NULL;) {
            sd = tmp -> socketFD;
            if(sd > 0) {
                FD_SET( sd , &readfds);
            }
            if(sd > max_sd) {
                max_sd = sd;
            }

            tmp = (client*) tmp -> next;
        }        
        /*for ( i = 0 ; i < max_clients ; i++) {
            //socket descriptor
            sd = client_socket[i];
             
            //if valid socket descriptor then add to read list
            if(sd > 0)
                FD_SET( sd , &readfds);
             
            //highest file descriptor number, need it for the select function
            if(sd > max_sd)
                max_sd = sd;
        }*/
  
        //wait for an activity on one of the sockets , timeout is NULL , so wait indefinitely
        activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
    
        if ((activity < 0) && (errno!=EINTR)) {
            printf("select error");
        }
          
        //If something happened on the master socket , then its an incoming connection
        if (FD_ISSET(master_socket, &readfds)) {
            if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
                perror("accept");
                exit(EXIT_FAILURE);
            }
          
            //inform user of socket number - used in send and receive commands
            printf("New connection , socket fd is %d , ip is : %s , port : %d \n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
            
            //add new socket to array of sockets
            /*for (i = 0; i < max_clients; i++) {
                //if position is empty
                if( client_socket[i] == 0 ) {
                    client_socket[i] = new_socket;
                    printf("Adding to list of sockets as %d\n" , i);
                     
                    break;
                }
            }*/
            client* newClient = (client*) malloc(sizeof(client));
            newClient -> socketFD = new_socket;
            newClient -> next = NULL;
            newClient -> firstMsg = 1;
            pushClient(&connectedClients, &newClient);
        }
          
        //else its some IO operation on some other socket :)
        //for (i = 0; i < max_clients; i++) {
          //  sd = client_socket[i];
        for (tmp = connectedClients; tmp != NULL;) {
            sd = tmp -> socketFD;
            if (FD_ISSET( sd , &readfds)) {
                //Check if it was for closing , and also read the incoming message
                if ((valread = read( sd , buffer, 1024)) == 0) {
                    //Somebody disconnected , get his details and print
                    /*getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    close(sd);
                    //client_socket[i] = 0;
                    removeClient(&connectedClients, sd);*/
                    disconnectClient(&connectedClients, sd);
                    sprintf(buffer, "> @%s lost connection", tmp -> nick);
                    sendMessageToAll(connectedClients, buffer);
                } else { //Echo back the message that came in
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    printf("%s\n", buffer);
                    //send(sd , buffer , strlen(buffer) , 0 );
                    if (tmp -> firstMsg) {
                        strcpy(tmp -> nick, buffer);
                        tmp -> firstMsg = 0;
                        if (checkNickname(connectedClients, tmp)) {
                            printf("%s has connected\n", tmp -> nick);
                            sprintf(buffer, "> @%s has joined the chat", tmp -> nick);
                            sendMessageToAllExcept(connectedClients, buffer, sd);
                        } else {
                            sprintf(buffer, "Sorry '%s' is already connected, please change nickname and retry...", tmp -> nick);
                            sendMessageTo(sd, buffer);
                            //sprintf(buffer, "Sorry '%s' is already connected, please change nickname and retry...", buffer);
                            disconnectClient(&connectedClients, sd);
                        }                        
                    } else {
                        if (strncmp(buffer, "#quit", 5) == 0) {
                        //if (strcmp(buffer, "#quit") == 0) {
                            disconnectClient(&connectedClients, sd);
                            sprintf(buffer, "> @%s has left the chat", tmp -> nick);
                            sendMessageToAll(connectedClients, buffer);
                        } else if (strncmp(buffer, "#showall", 8) == 0) {
                        //} else if (strcmp(buffer, "#showall") == 0) {
                            getAllClients(connectedClients, buffer);
                            sendMessageTo(sd, buffer);
                        } else if (strncmp(buffer, "@", 1) == 0) {
                            sendPrivateMessage(tmp, buffer, connectedClients);
                        } else {
                            //sprintf(auxBuffer, "%s> %s", tmp -> nick, buffer);
                            //auxBuffer[strlen(tmp -> nick) + strlen(buffer) + 2] = '\0';
                            //strncpy(auxBuffer, buffer, strlen(buffer));
                            //sprintf(buffer, " %s> %s ", tmp -> nick, auxBuffer);
                            //printf("%d\n", strlen(auxBuffer));
                            //sendMessageToAll(connectedClients, buffer);
                            sendCommonMessage(tmp, buffer, connectedClients);
                        }
                    }
                }
            }
            tmp = (client*) tmp -> next;
        }
    }
    return 0;
}