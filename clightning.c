#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define RESISTANCE_MAX 10

enum dir {
	NONE = 0,
	NORTH = 0x8,
	EAST = 0x4,
	SOUTH = 0x2,
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
	unsigned dir = 0;

	if (y < 0) {
		dir |= NORTH;
	} else if (y > 0) {
		dir |= SOUTH;
	}

	if (x > 0) {
		dir |= EAST;
	} else if (x < 0) {
		dir |= WEST;
	}

	return dir;
}

void bolt(char **canvas, int **resistance, int **sky, int xmax, int ymax, int x, int y, int len, int lastx, int lasty) {
	do {
		// pick the path(s) of least resistance
		int m = RESISTANCE_MAX; // TODO DCB always larger than the highest resistance value
		int xmin, ymin;

		for (int i = x - 1; i < x + 1; ++i) {
			if (i < 0 || i >= xmax) {
				return;
			}

			for (int j = y - 1; j < y + 1; ++j) {
				if (j < 0 || j >= ymax) {
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
					m = r;
					xmin = i;
					ymin = j;
				}

				if (r < 2) {
					bolt(canvas, resistance, sky, xmax, ymax, i, j, len / 2, x, y);
				}
			}
		}

		char c = chars[get_direction(x - xmin, y - ymin)];
		if (canvas[x][y] == ' ') {
			canvas[x][y] = c;

			for (int i = x - 5; i < x + 5; ++i) {
				if (i < 0 || i >= xmax) {
					return;
				}

				for (int j = y - 5; j < y + 5; ++j) {
					if (j < 0 || j >= ymax) {
						return;
					}
					sky[i][j] = sky[i][j] + 1;
				}
			}
		}
		lastx = x;
		lasty = y;
		x = xmin;
		y = ymin;
		len -= m;
	} while (len > 0);
}

int main(int argc, char **argv) {
	// Enter curses mode
	initscr();
	timeout(100);

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
	int **sky = malloc( x * sizeof(int *));
	int **resistance = malloc(x * sizeof(int *));
	for (int i = 0; i < x; ++i) {
		canvas[i] = malloc(y * sizeof(char));
		resistance[i] = malloc(y * sizeof(int));
		sky[i] = malloc(y * sizeof(int));
		memset(canvas[i], ' ', y);
		memset(sky[i], 0, y);
		for (int j = 0; j < y; ++j) {
			resistance[i][j] = rand() % RESISTANCE_MAX;
		}
	}

	// pick a random spot to start in the middle 50% of the window
	int x0 = (x / 4) + (rand() % (x / 2));
	int y0 = (y / 4) + (rand() % (y / 2));
	int len = rand() % (x + y);

	bolt(canvas, resistance, sky, x, y, x0, y0, len, -1 /* lastx */, -1 /* lasty */);

	WINDOW *bolt;
	WINDOW *blank;
	bolt = newwin(y, x, 0, /* starty */ 0 /* startx */);
	blank = newwin(y, x, 0, /* starty */ 0 /* startx */);
	wclear(blank);
	wclear(bolt);

	for (int i = 0; i < 3; ++i) {
		for (int i = 0; i < x; ++i) {
			for (int j = 0; j < y; ++j) {
				char c = canvas[i][j];
				if (c != ' ') {
					wattron(bolt, A_BOLD);
					mvwaddch(bolt, j, i, c);
					wattroff(bolt, A_BOLD);
				} else {
					int brightness = sky[i][j];
					wattron(bolt, A_DIM);
					if (brightness > 9) {
						mvwaddch(bolt, j, i, '*');
					} else if (brightness >= 4) {
						mvwaddch(bolt, j, i, '\'');
					} else {
						mvwaddch(bolt, j, i, ' ');
					}
					wattroff(bolt, A_DIM);
				}
			}
		}
	}
	wmove(bolt, 0, 0);

	int flashes = 2 + rand() % 6;
	for (int i = 0; i < flashes; ++i) {
		int on = 10000 + (rand() % (100 * 1000));
		int off = 10000 + (rand() % (100 * 1000));
		redrawwin(bolt);
		wrefresh(bolt);
		usleep(on);
		redrawwin(blank);
		wrefresh(blank);
		usleep(off);
	}

	wtimeout(blank, 1000);
	wgetch(blank);


	delwin(bolt);
	delwin(blank);

	// Exit curses mode
	endwin();

	// Hide the memory
	for (int i = 0; i < x; ++i) {
		free(canvas[i]);
		free(resistance[i]);
		free(sky[i]);
	}
	free(canvas);
	free(resistance);
	free(sky);

	return 0;
}
