#include <stdio.h>
#include <ncurses.h>
#include <string.h>

char nameTemp[160];
char array[18][1025], pos = 0;
WINDOW* local_win;

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

void drawGreen() {
	attron(COLOR_PAIR(2));
	wborder(local_win, '<', '>', '^','v','*','*','*','*'); //Draw interface
	wbkgd(local_win, COLOR_PAIR(2));
	mvprintw(0, 14, "== AppWhats 2 TM - Just plain chatting, literally =="); //Draws title
	mvprintw(20, 1, "==============================================================================");
	attroff(COLOR_PAIR(2));
}

int main() {

	initscr();
	raw();
	start_color();
	local_win = newwin(24, 80, 0, 0);
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	attron(COLOR_PAIR(1));
	drawLogo();
	attroff(COLOR_PAIR(1));
	getch(); //Press any key 2 advance
	drawGreen(); 

	while(1) {
		drawGreen();
		attron(COLOR_PAIR(1)); //Black background for user writing.
		mvprintw(21, 1, "                                                                              ");
		mvprintw(22, 1, "                                                                              ");
		attroff(COLOR_PAIR(1));
		wrefresh(local_win);

		move(21, 3);
	    getstr(nameTemp); //Start chatting
	    printBuffer(nameTemp);

//	    if (getch() == ']')
//	    	endwin();
	}
	return 0;
}