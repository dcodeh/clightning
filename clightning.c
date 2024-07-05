#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

union direction {
	unsigned val : 4;
	struct {
		unsigned north : 1;
		unsigned east : 1;
		unsigned south : 1;
		unsigned west : 1;
	} bits;
};

enum dir {
	NONE = 0,
	NORTH = 0x8,
	EAST = 0x2,
	SOUTH = 0x4,
	WEST = 0x1,
};

static char chars[] = {
	[NONE] = '*',
	[SOUTH] = '|',
	[NORTH] = '|',
	[NORTH | EAST] = '/',
	[SOUTH | WEST] = '/',
	[SOUTH | EAST] = '\\',
	[NORTH | WEST] = '\\',
	[EAST] = '-',
	[WEST] = '-',
};

unsigned get_direction(int x, int y) {
	union direction dir;

	dir.bits.north = (x < 0);
	dir.bits.south = (x > 0);
	dir.bits.east = (y > 0);
	dir.bits.west = (y < 0);

	return dir.val;
}

void bolt(char **canvas, int **resistance, int xmax, int ymax, int x, int y, int len) {
	if (len <= 0) {
		return;
	}

	// pick the path(s) of least resistance
	int m = 10; // TODO DCB always larger than the highest resistance value
	int xmin, ymin;

	for (int i = x - 1; i > 0 && i < xmax && i < x + 1; ++i) {
		for (int j = y - 1; j > 0 && j < ymax && j < y + 1; ++j) {
			int r = resistance[i][j];
			if (r < m) {
				m = r;
				xmin = i;
				ymin = j;
			}
		}
	}

	canvas[x][y] = chars[get_direction(x - xmin, y - ymin)];
	bolt(canvas, resistance, xmax, ymax, xmin, ymin, len - 1);
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

	// pick a random spot to start in the middle 50% of the window
	int x0 = (x / 4) + (rand() % (x / 2));
	int y0 = (y / 4) + (rand() % (y / 2));
	int len = rand() % (x + y);

	bolt(canvas, resistance, x, y, x0, y0, len);

	// Output the contents of the array
	// Go home (you're drunk)
	move(0, 0);
	attron(A_BOLD);

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			// mvaddch(j, i, resistance[i][j] + '0');
			mvaddch(j, i, canvas[i][j]);
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
