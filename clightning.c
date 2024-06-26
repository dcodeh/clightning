#include <stdio.h>
#include <ncurses.h>

int main(int argc, char **argv) {
	// Enter curses mode
	initscr();

	// Disable character buffering
	raw();

	// Disable echoing input characters that could mess up the artwork
	noecho();

	// Enquire about the size of the terminal we're dealing with
	int x, y;
	getmaxyx(stdscr, y, x);

	attron(A_BOLD);
	printw("LIGHTNING");

	// Print window size in the center
	mvprintw(y / 2, x / 2, "%d x %d", x, y);
	refresh();

	getch();

	// Exit curses mode
	endwin();

	return 0;
}
