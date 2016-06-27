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

int sendMessageToAll(client* list, char* buffer) {
    client* tmp = list;

    if (tmp == NULL) {
        return 0;
    }

    for (; tmp != NULL; ) {
        send(tmp -> socketFD, buffer, strlen(buffer), 0);
        tmp = (client*)(tmp->next);
    }

    return 1;
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
      
    //set of socket descriptors
    fd_set readfds;
      
    //a message
    char *message = "ECHO Daemon v1.0 \r\n";
  
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
    puts("Waiting for connections ...");
     
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
        
            //send new connection greeting message
            if( send(new_socket, message, strlen(message), 0) != strlen(message) ) {
                perror("send");
            }
              
            puts("Welcome message sent successfully");
              
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
                    getpeername(sd , (struct sockaddr*)&address , (socklen_t*)&addrlen);
                    printf("Host disconnected , ip %s , port %d \n" , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
                      
                    //Close the socket and mark as 0 in list for reuse
                    close( sd );
                    //client_socket[i] = 0;
                    removeClient(&connectedClients, sd);
                    printf("Removed client %d\n", sd);
                } else { //Echo back the message that came in
                    //set the string terminating NULL byte on the end of the data read
                    buffer[valread] = '\0';
                    //send(sd , buffer , strlen(buffer) , 0 );
                    sendMessageToAll(connectedClients, buffer);
                }
            }
            tmp = (client*) tmp -> next;
        }
    }
    return 0;
}