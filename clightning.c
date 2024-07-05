#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static const char lightning_chars[] = {
	'-',
	'\\',
	'/',
	'|',
};

size_t num_chars = sizeof(lightning_chars) / sizeof(char); // TODO DCB marcro for this?

char rndchar() {
	int r = rand() % num_chars;
	return lightning_chars[r];
}

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

	srand(time(NULL));

	// TODO DCB clean up this hacky mess
	char **canvas = malloc(x * sizeof(char *));
	int **resistance = malloc(x * sizeof(int *));
	for (int i = 0; i < x; ++i) {
		canvas[i] = malloc(y * sizeof(char));
		resistance[i] = malloc(y * sizeof(int));
		memset(canvas[i], ' ', y);
		for (int j = 0; j < y; ++j) {
			resistance[i][j] = rand() % 5;
		}
	}

	// TODO DCB pick a random spot to start in the middle 50% of the window
	int x0 = (x / 4) + (rand() % (x / 2));
	int y0 = (y / 4) + (rand() % (y / 2));

	// Output the contents of the array
	// Go home (you're drunk)
	move(0, 0);
	attron(A_BOLD);

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			mvaddch(j, i, resistance[i][j] + '0');
		}
	}

	refresh();
	getch();

	// Exit curses mode
	endwin();

	// Hide the memory
	for (int i = 0; i < x; ++i) {
		free(canvas[i]);
		free(resistance[i]);
	}
	free(canvas);
	free(resistance);

	return 0;
}
