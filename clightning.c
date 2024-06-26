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

	char canvas[x][y];

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			canvas[i][j] = '@';
		}
	}

	// Go home
	move(0, 0);

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			addch(canvas[i][j]);
		}
	}

	attron(A_BOLD);
	refresh();

	getch();

	// Exit curses mode
	endwin();

	return 0;
}
