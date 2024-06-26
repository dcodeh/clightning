#include <stdio.h>
#include <ncurses.h>

int main(int argc, char **argv) {
	initscr();
	printw("LIGHTNING");
	refresh();
	getch();
	endwin();

	return 0;
}
