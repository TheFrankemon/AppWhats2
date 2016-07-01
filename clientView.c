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
char array[18][1025], pos = 0;
WINDOW* write_win;
WINDOW* read_win;

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

void scrollBuffer() {
	for (int c = 0 ; c < 17 ; c++ ) {
		strcpy(array[c], array[c + 1]);
	}
	pos = 16;
}

void printBuffer(char *msg) {
	if (pos > 16) {
		scrollBuffer();
	}

	strcpy(array[pos], msg);
	pos++;

/*
	int len = strlen( array[0] );
	for (int i = 0; i < len; i++) {
			mvprintw(15, 30, &array[0][i]);
		if (strcmp(&array[0][i],">") == 0) {
			attron(COLOR_PAIR(1));
		}
		mvprintw(20, 20, array[0]);
	}
*/
	for(int c = 0 ; c < pos ; c++ ) {
		mvprintw(2 + c, 5, array[c]);
	}
}

void paintReadWindow() {
	attron(COLOR_PAIR(2));
	wborder(read_win, '<', '>', '^','-','*','*','*','*');
	wbkgd(read_win, COLOR_PAIR(2));
	mvprintw(0, 14, "== AppWhats 2 TM - Just plain chatting, literally ==");
	attroff(COLOR_PAIR(2));
	move(21, 3);
}

void paintWriteWindow() {
	attron(COLOR_PAIR(2));
	wborder(write_win, '<', '>', '-','v','*','*','*','*');
	wbkgd(write_win, COLOR_PAIR(2));
	attroff(COLOR_PAIR(2));
	attron(COLOR_PAIR(1)); //Black background for user writing.
	mvprintw(21, 1, "                                                                              ");
	mvprintw(22, 1, "                                                                              ");
	attroff(COLOR_PAIR(1));
	move(21, 3);
}

void validateArgs(int argc, char *argv[]) {
	if (argc != 3) {
		perror("error: must give 2 params (server_ip server_port)");
		exit(EXIT_FAILURE);
	}
}

int main(int argc , char *argv[])
{
	char *serverIP = argv[1], *err;
	struct sockaddr_in stSockAddr;
	int activity, valread, max_sd;
	char buffer[1025];  //data buffer of 1K
	fd_set readfds; 	//set of socket descriptors
	
	validateArgs(argc, argv);
	
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

	memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

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
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	drawLogo();

	system("/bin/stty raw"); 				//Kills buffering

	while(1) {
		FD_ZERO(&readfds);					//clear the socket set
		FD_SET(socketFD, &readfds);			//add server socket to set
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