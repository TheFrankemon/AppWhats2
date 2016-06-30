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
WINDOW* local_win;
WINDOW* top_win;

void drawLogo() {
	mvprintw(8,8,"        /\\            \\            / /        /       #######");
	mvprintw(9,8,"       /  \\            \\          / /       _/_          ### ");
	mvprintw(10,8,"      /____\\    ___ ___ \\        / /__  __  /  ____  ######  ");
	mvprintw(11,8,"     /      \\  /  //  /  \\  /\\  / /  / __/ /  /_    ##       ");
	mvprintw(12,8,"    /        \\/__//__/    \\/  \\/ /  / /_/ /_ __/   #######   ");
	mvprintw(13,8,"             /   /                                           ");
	mvprintw(14,8,"____________/   /____________________________________________");
	mvprintw(15,8,"__________PRESS ANY KEY TO CHAT LIKE A REAL DEV :v___________");
}
 

void scrollBuffer() {
	for (int c = 0 ; c < 18 ; c++ ) {
		strcpy(array[c], array[c + 1]);
	}
	pos = 17;
}

void printBuffer(char *msg) {
	if (pos > 17) {
		scrollBuffer();
	}

	strcpy(array[pos], msg);
	pos++;

	for(int c = 0 ; c < pos ; c++ ) {
		mvprintw(2 + c, 5, array[c]);
	}
}

void clearBottom() {
	attron(COLOR_PAIR(1)); //Black background for user writing.
	mvprintw(21, 1, "                                                                              ");
	mvprintw(22, 1, "                                                                              ");
	attroff(COLOR_PAIR(1));
	move(21, 3);
}

void drawTopGreen() {
	attron(COLOR_PAIR(2));
	wborder(top_win, '<', '>', '^','v','*','*','*','*'); //Draw interface
	wbkgd(top_win, COLOR_PAIR(2));
	mvprintw(0, 14, "== AppWhats 2 TM - Just plain chatting, literally =="); //Draws title
	mvprintw(20, 1, "==============================================================================");
	attroff(COLOR_PAIR(2));
	move(21, 3);
}

void drawGreen() {
	attron(COLOR_PAIR(2));
	wborder(local_win, '<', '>', '^','v','*','*','*','*'); //Draw interface
	wbkgd(local_win, COLOR_PAIR(2));
	mvprintw(0, 14, "== AppWhats 2 TM - Just plain chatting, literally =="); //Draws title
	mvprintw(20, 1, "==============================================================================");
	attroff(COLOR_PAIR(2));
	move(21, 3);
}

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

	//freopen("/dev/tty", "rw", stdin);
	initscr();
	raw();
	start_color();
	local_win = newwin(24, 80, 0, 0);
	top_win = newwin(21, 80, 0, 0);
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	attron(COLOR_PAIR(1));
	drawLogo();
	attroff(COLOR_PAIR(1));
	getch(); //Press any key 2 advance
	//drawGreen();

	drawGreen();
	attron(COLOR_PAIR(1)); //Black background for user writing.
	mvprintw(21, 1, "                                                                              ");
	mvprintw(22, 1, "                                                                              ");
	attroff(COLOR_PAIR(1));
	wrefresh(local_win);
	//nodelay(stdscr,TRUE);
	//keypad(stdscr, FALSE);
	echo();
	nocbreak();
	//timeout(500);
	move(21, 3);

	//printf("odiafiosdihfaohisdf\n");

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
	
	while(1) {

		//clear the socket set
		FD_ZERO(&readfds);
		
		//add server socket to set
		FD_SET(socketFD, &readfds);
		FD_SET(STDIN, &readfds);

		
		//getstr(nameTemp);
		//endwin();
		refresh();
		//getstr(buffer);
		move(21, 3);
		
		//printf("I'm here");
		//mvprintw(21, 3, "I'm here");
		//fflush(stdout);
		//fflush(stdin);
		
		move(21, 3);
		//getstr(buffer);
		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
		//printw("Now I'm here");
		//refresh();
		
		if ((activity < 0) && (errno!=EINTR)) {
			printf("select error");
		}
		
		//Server activity
		if (FD_ISSET(socketFD, &readfds)) {
			valread = read(socketFD, buffer, 1024);
			buffer[valread] = '\0';
			//printf("Msg: %s\n----------------------\n", buffer);
			if (valread == 0) {
				endwin();
				exit(EXIT_SUCCESS);
			} else {
				//printf("%s\n", buffer);
				drawTopGreen();
				printBuffer(buffer);
				wrefresh(top_win);
			}
		}
		
		//STDIN activity
		if (FD_ISSET(STDIN, &readfds)) {
			valread = read(STDIN, buffer, 1024);
			buffer[valread] = '\0';
			send(socketFD, buffer , strlen(buffer) , 0);
			clearBottom();
			refresh();
			mvprintw(21, 3, buffer);
			fflush(stdout);

			/*if (strncmp(buffer, "#quit", 5) == 0) {
				exit(EXIT_SUCCESS);
			}*/
		}
	}

	//getch();
	endwin();
	exit(EXIT_FAILURE);
}
