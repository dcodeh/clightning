#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#define RESISTANCE_MAX 10

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
	[NORTH | WEST] = '\\',
	[SOUTH | WEST] = '/',
	[SOUTH | EAST] = '\\',
	[EAST] = '-',
	[WEST] = '-',
};

unsigned get_direction(int x, int y) {
	union direction dir;

	dir.bits.north = (y < 0);
	dir.bits.south = (y > 0);
	dir.bits.east = (x > 0);
	dir.bits.west = (x < 0);

	return dir.val;
}

void bolt(char **canvas, int **resistance, int xmax, int ymax, int x, int y, int len) {
	int lastx = -1;
	int lasty = -1;

	while (len-- > 0) {
		// pick the path(s) of least resistance
		int m = RESISTANCE_MAX; // TODO DCB always larger than the highest resistance value
		int xmin, ymin;

		for (int i = x - 1; i < x + 1; ++i) {
			for (int j = y - 1; j < y + 1; ++j) {
				if (i < 0 || i >= xmax || j < 0 || j >= ymax) {
					// skip invalid indexes (around the edges and such)
					return;
				}

				if (lastx > 0 && lasty > 0) {
					if (i == lastx && j == lasty) {
						// skip the place we came from
						continue;
					}
				}

				if (i == x && j == y) {
					// skip center position
					continue;
				}

				int r = resistance[i][j];
				if (r < m) {
					printf("%d < %d\n", r, m);
					m = r;
					xmin = i;
					ymin = j;
				} else if (r == m) {
					printf("RECURSION-URSION-ursion-ursion-u r s i o n-...\n");
					bolt(canvas, resistance, xmax, ymax, i, j, len / 2);
				}
			}
		}

		char c = chars[get_direction(x - xmin, y - ymin)];
		if (canvas[x][y] != ' ') {
			// deja vu
			return;
		}
		canvas[x][y] = c;
		lastx = x;
		lasty = y;
		x = xmin;
		y = ymin;
		printf("Placed %c at %d, %d\n", c, lastx, lasty);
	}
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
			int r = rand() % (2 * RESISTANCE_MAX);
			if (r >= RESISTANCE_MAX) {
				r = RESISTANCE_MAX - 1;
			}
			resistance[i][j] = r;
		}
	}

	// pick a random spot to start in the middle 50% of the window
	int x0 = (x / 4) + (rand() % (x / 2));
	int y0 = (y / 4) + (rand() % (y / 2));
	int len = rand() % (2 * x + y);

	bolt(canvas, resistance, x, y, x0, y0, len);

	// Output the contents of the array
	// Go home (you're drunk)
	move(0, 0);

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			char c = canvas[i][j];
			if (c != ' ') {
				attron(A_BOLD);
				mvaddch(j, i, c);
			} else {
				attroff(A_BOLD);
				mvaddch(j, i, resistance[i][j] + '0');
			}
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
