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
#include <ncurses.h>

#define TRUE   1
#define FALSE  0
#define PORT 8888
#define STDIN 0

char nameTemp[160];
/**
 * Array of 17 slots, saves the last sent messages (discards the older ones)
 */
char msgsCache[17][1025];
char pos = 0;
/**
 * 2 windows inside the UI, one of top of the other.
 * write_win shows the messages saved on server.
 * read_win shows a "text box" for the user input.
 */
WINDOW* write_win;
WINDOW* read_win;

/**
 * @brief      Validates the program recieves both server IP and server Port.
 *
 * @param[in]  argc  Number of parameters
 * @param      argv  Array of strings
 */
void validateArgs(int argc, char *argv[]) {
	if (argc != 3) {
		perror("error: must give 2 params (server_ip server_port)");
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief      Draws the program's splash screen.
 */
void drawLogo() {
	attron(COLOR_PAIR(1));

	mvprintw(8,8,"        /\\            \\            / /        /       #######");
	mvprintw(9,8,"       /  \\            \\          / /       _/_          ### ");
	mvprintw(10,8,"      /____\\    ___ ___ \\        / /__  __  /  ____  ######  ");
	mvprintw(11,8,"     /      \\  /  //  /  \\  /\\  / /  / __/ /  /_    ##       ");
	mvprintw(12,8,"    /        \\/__//__/    \\/  \\/ /  / /_/ /_ __/   #######   ");
	mvprintw(13,8,"             /   /                                           ");
	mvprintw(14,8,"____________/   /____________________________________________");
	mvprintw(15,7,"_________________IT'S TIME TO CHAT LIKE A REAL DEV :v___________");

	mvprintw(20, 24, "'Allo, 'allo, my name is ");
	attroff(COLOR_PAIR(1));
}

/**
 * @brief      Draws/repaints the window that shows the messages.
 */
void paintReadWindow() {
	attron(COLOR_PAIR(2));
	wborder(read_win, '<', '>', '^','-','*','*','*','*');
	wbkgd(read_win, COLOR_PAIR(2));
	mvprintw(0, 14, "== AppWhats 2 TM - Just plain chatting, literally ==");
	attroff(COLOR_PAIR(2));
	move(21, 3);
}

/**
 * @brief      Draws/repaints the window that gets the user input.
 */
void paintWriteWindow() {
	attron(COLOR_PAIR(2));
	wborder(write_win, '<', '>', '-','v','*','*','*','*');
	wbkgd(write_win, COLOR_PAIR(2));
	attroff(COLOR_PAIR(2));
	attron(COLOR_PAIR(1));					//Draws a black "text box".
	mvprintw(21, 1, "                                                                              ");
	mvprintw(22, 1, "                                                                              ");
	attroff(COLOR_PAIR(1));
	move(21, 3);
}

/**
 * @brief      Deletes the oldest message to make space for the newest.
 */
void scrollBuffer() {
	for (int c = 0 ; c < 17 ; c++ ) {
		strcpy(msgsCache[c], msgsCache[c + 1]);
	}
	pos = 16;
}

/**
 * @brief      Verifies that msgsCache is not overflown.
 * 				Adds the newest message to the msgsCache, and then prints its contents.
 * 				Changes color to the server-sent messages.
 *
 * @param      msg   The message that was just written.
 */
void printBuffer(char *msg) {
	if (pos > 16) {
		scrollBuffer();
	}

	strcpy(msgsCache[pos], msg);
	pos++;

	for(int c = 0 ; c < pos ; c++ ) {
		if (strncmp(msgsCache[c], ">", 1) == 0) {
			attron(COLOR_PAIR(3));
		}
		mvprintw(2 + c, 5, msgsCache[c]);
		attroff(COLOR_PAIR(3));
	}
}

int main(int argc , char *argv[])
{
	char *serverIP = argv[1], *err;		//saves the server's IP
	struct sockaddr_in stSockAddr;
	int activity, valread, max_sd;		//used for select
	char buffer[1025];  				//data buffer of 1K
	fd_set readfds; 					//set of socket descriptors
	
	validateArgs(argc, argv);
	
	int serverPort = strtol(argv[2], &err, 10); 	// converts port string to int
    if (err[0] != '\0') { 							// bad input, not a number
    	perror("error: char string (invalid port)");
    	exit(EXIT_FAILURE);
    }

    // creates a socket for the server connection
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);	// IPPROTO_TCP);
	if (-1 == socketFD) {
		perror("cannot create socket");
		exit(EXIT_FAILURE);
	}

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));		//Saves a space in memory for the socket

    //type of socket created
	stSockAddr.sin_family = AF_INET;
	stSockAddr.sin_port = htons(serverPort);
	int res = inet_pton(AF_INET, serverIP, &stSockAddr.sin_addr);

	if (0 > res) {
		perror("error: first parameter is not a valid address family");
		close(socketFD);
		exit(EXIT_FAILURE);
	} else if (0 == res) {
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

	//calculates max file descriptor
	max_sd = (socketFD > STDIN) ? socketFD : STDIN;

	initscr();
	raw();
	start_color();
	//Defines the color palette with 3 flavors.
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);
	drawLogo();

	system("/bin/stty raw"); 				//Kills buffering

	//Waits for the user's nickname to continue
	while(1) {
		FD_ZERO(&readfds);					//clear the socket set
		FD_SET(socketFD, &readfds);			//add server socket to set
		FD_SET(STDIN, &readfds);			//add socket for user input

		refresh();
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) {
			printf("select error");
		}
		
		//Server activity
		if (FD_ISSET(socketFD, &readfds)) {
			valread = read(socketFD, buffer, 1024);
			buffer[valread] = '\0';
			if (valread == 0) {
				endwin();
				exit(EXIT_SUCCESS);
			} else {
				if (strncmp(buffer, "Welcome!", 8) == 0) {
					break;
				}
				mvprintw(23, 8, buffer);
			}
		}
		
		//STDIN activity
		if (FD_ISSET(STDIN, &readfds)) {
			getstr(buffer);
			send(socketFD, buffer , strlen(buffer), 0);
			refresh();
		}
	}

	//If everything is OK, then "open" the chat UI.
	read_win = newwin(21, 80, 0, 0);
	write_win = newwin(4, 80, 20, 0);
	paintReadWindow();
	paintWriteWindow();
	wrefresh(read_win);
	wrefresh(write_win);
	echo();
	nocbreak();
	move(21, 3);
	system("/bin/stty raw");

	//Waits continously for the user's input messages to be sent to the server.
	while(1) {
		FD_ZERO(&readfds);
		FD_SET(socketFD, &readfds);
		FD_SET(STDIN, &readfds);
		refresh();
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		
		if ((activity < 0) && (errno!=EINTR)) {
			printf("select error");
		}
			
		//Server activity
		if (FD_ISSET(socketFD, &readfds)) {
			valread = read(socketFD, buffer, 1024);
			buffer[valread] = '\0';
			if (valread == 0) {
				endwin();
				exit(EXIT_SUCCESS);
			} else {
				paintReadWindow();
				printBuffer(buffer);
				wrefresh(read_win);
			}
		}
		
		//STDIN activity
		if (FD_ISSET(STDIN, &readfds)) {
			move(21, 3);
			getstr(buffer);
			send(socketFD, buffer , strlen(buffer), 0);
			paintWriteWindow();
			refresh();
		}
	}

	endwin();
	exit(EXIT_FAILURE);
}