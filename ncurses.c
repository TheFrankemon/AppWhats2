#include <stdio.h>
#include <ncurses.h>

char nameTemp[160];
char * array[50], c, pos, cap;

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
 
/*
void scrollBuffer() {
	for ( c = position - 1 ; c < n - 1 ; c++ )
    	array[c] = array[c+1]; 
}
*/
void printBuffer(char* msg) {
	array[pos] = msg;
	//for( c = 0 ; c < 9 ; c++ )
	mvprintw( 4, 3, array[0]);
	mvprintw( 5, 3, array[1]);
	mvprintw( 6, 3, array[2]);
}

int main() {

	initscr();
	raw();
	start_color();
	WINDOW* local_win = newwin(24, 80, 0, 0);
	init_pair(1, COLOR_GREEN, COLOR_BLACK);
	init_pair(2, COLOR_BLACK, COLOR_GREEN);
	attron(COLOR_PAIR(1));
	drawLogo();
	attroff(COLOR_PAIR(1));
	attron(COLOR_PAIR(2));
	getch(); //Press any key 2 advance
		wborder(local_win, '{', '}', '^','v','*','*','*','*'); //Draw interface
		wbkgd(local_win, COLOR_PAIR(2));
		mvprintw(0, 14, "== AppWhats 2 TM - Just plain chatting, literally =="); //Draws title
		mvprintw(18, 1, "==============================================================================");
		attroff(COLOR_PAIR(2));

	pos = 0;
	for (; ;)	{
		attron(COLOR_PAIR(1)); //Black background for user writing.
		mvprintw(19, 1, "                                                                              ");
		mvprintw(20, 1, "                                                                              ");
		mvprintw(21, 1, "                                                                              ");
		mvprintw(22, 1, "                                                                              ");
		attroff(COLOR_PAIR(1));
		wrefresh(local_win);

		move(19, 3);
	    getstr(nameTemp); //Start chatting
	    printBuffer(nameTemp);
		pos = pos + 1;

//	    if (getch() == ']')
//	    	endwin();
	}
	return 0;
}