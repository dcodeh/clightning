#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define RESISTANCE_MAX 10
#define RECURSION_RESISTANCE 2
#define GLOW_RADIUS 3

#define MIN_FLASHES 2
#define MAX_FLASHES 6

// microseconds
#define FLASH_ON_MAX (100 * 1000)
#define FLASH_OFF_MAX (100 * 1000)
#define FLASH_ON_MIN 10000
#define FLASH_OFF_MIN 10000

// milliseconds
#define INPUT_TIMEOUT 1000

// glow settings
#define INTENSE_THRESHOLD 8
#define INTENSE_CHAR '@'
#define MEDIUM_THRESHOLD 4
#define MEDIUM_CHAR  '*'
#define LOW_THRESHOLD 2
#define LOW_CHAR '.'

enum color_pair {
	BOLT_PAIR = 1,
	INTENSE_GLOW_PAIR,
	MEDIUM_GLOW_PAIR,
	LOW_GLOW_PAIR,
	NO_GLOW_PAIR
};

enum dir {
	NONE = 0,
	WEST = 0x1,
	SOUTH = 0x2,
	EAST = 0x4,
	NORTH = 0x8,
};

// A crappy lookup table
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

void w_put_char(WINDOW *w, int x, int y, char c, unsigned attribute) {
	wattron(w, attribute);
	mvwaddch(w, y, x, c);
	wattroff(w, attribute);
}

void bolt_to_window(WINDOW *w, char **canvas, int **sky, int xmax, int ymax) {
	for (int i = 0; i < xmax; ++i) {
		for (int j = 0; j < ymax; ++j) {
			char c = canvas[i][j];
			if (c != ' ') {
				w_put_char(w, i, j, c,
						COLOR_PAIR(BOLT_PAIR) |
						A_BOLD);
			} else {
				int brightness = sky[i][j];
				wattron(w, A_DIM);
				if (brightness > INTENSE_THRESHOLD) {
					w_put_char(w, i, j, INTENSE_CHAR, COLOR_PAIR(INTENSE_GLOW_PAIR));
				} else if (brightness >= MEDIUM_THRESHOLD) {
					w_put_char(w, i, j, MEDIUM_CHAR, COLOR_PAIR(MEDIUM_GLOW_PAIR));
				} else if (brightness >= LOW_THRESHOLD) {
					w_put_char(w, i, j, LOW_CHAR, COLOR_PAIR(LOW_GLOW_PAIR));
				} else {
					w_put_char(w, i, j, ' ', COLOR_PAIR(NO_GLOW_PAIR));
				}
				wattroff(w, A_DIM);
			}
		}
	}
	wmove(w, 0, 0);
}

void color_sky(int **sky, int x, int y, int xmax, int ymax, int radius) {
	for (int i = x - radius; i < x + radius; ++i) {
		if (i < 0 || i >= xmax) {
			return;
		}

		for (int j = y - radius; j < y + radius; ++j) {
			if (j < 0 || j >= ymax) {
				return;
			}
			sky[i][j] = sky[i][j] + 1;
		}
	}
}

void bolt(char **canvas, int **resistance, int **sky, int xmax, int ymax,
		int x, int y, int len, int lastx, int lasty) {
	do {
		// pick the path(s) of least resistance
		int m = RESISTANCE_MAX;
		int xmin, ymin;

		for (int i = x - 1; i < x + 1; ++i) {
			if (i < 0 || i >= xmax) {
				// skip out of bounds
				return;
			}

			for (int j = y - 1; j < y + 1; ++j) {
				if (j < 0 || j >= ymax) {
					// skip out of bounds
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

				if (r < RECURSION_RESISTANCE) {
					bolt(canvas, resistance, sky,
							xmax, ymax, i, j,
							len / 2,
							x, y);
				}
			}
		}

		char c = chars[get_direction(x - xmin, y - ymin)];
		if (canvas[x][y] == ' ') {
			canvas[x][y] = c;

			color_sky(sky, x, y, xmax, ymax, GLOW_RADIUS/* radius */);
		}
		lastx = x;
		lasty = y;
		x = xmin;
		y = ymin;
		len -= m;
	} while (len > 0);
}

// possible options:
// --nocolor
// --noglow
// --storm
// --noflash
int main(int argc, char **argv) {
	// Enter curses mode
	initscr();

	// Disable character buffering
	raw();

	// Disable echoing input characters that could mess up the artwork
	noecho();

	// Initialize color pairs, if supported by this terminal
	bool colorful = has_colors();
	if (colorful) {
		start_color();
		init_pair(BOLT_PAIR, COLOR_WHITE, COLOR_BLACK);
		init_pair(INTENSE_GLOW_PAIR, COLOR_WHITE, COLOR_BLACK);
		init_pair(MEDIUM_GLOW_PAIR, COLOR_CYAN, COLOR_BLACK);
		init_pair(LOW_GLOW_PAIR, COLOR_BLUE, COLOR_BLACK);
		init_pair(NO_GLOW_PAIR, COLOR_WHITE, COLOR_BLACK);
	}

	// Enquire about the size of the terminal we're dealing with
	int xmax, ymax;
	getmaxyx(stdscr, ymax, xmax);

	// Seed random generator with the current time
	srand(time(NULL));

	// Acquire lots of memory
	char **canvas = malloc(xmax * sizeof(char *));
	int **sky = malloc(xmax * sizeof(int *));
	int **resistance = malloc(xmax * sizeof(int *));
	for (int i = 0; i < xmax; ++i) {
		canvas[i] = malloc(ymax * sizeof(char));
		resistance[i] = malloc(ymax * sizeof(int));
		sky[i] = malloc(ymax * sizeof(int));
		memset(canvas[i], ' ', ymax);
		memset(sky[i], 0, ymax);
		for (int j = 0; j < ymax; ++j) {
			resistance[i][j] = rand() % RESISTANCE_MAX;
		}
	}

	// pick a random spot to start in the middle 50% of the window
	int x0 = (xmax / 4) + (rand() % (xmax / 2));
	int y0 = (ymax / 4) + (rand() % (ymax / 2));
	int len = rand() % (xmax * ymax);

	// Create a lightning bolt
	bolt(canvas, resistance, sky, xmax, ymax, x0, y0, len,
			-1 /* lastx */, -1 /* lasty */);

	WINDOW *bolt;
	WINDOW *blank;
	bolt = newwin(ymax, xmax, 0, /* starty */ 0 /* startx */);
	blank = newwin(ymax, xmax, 0, /* starty */ 0 /* startx */);
	wclear(blank);
	wclear(bolt);

	// Write lightning bolt to window
	bolt_to_window(bolt, canvas, sky, xmax, ymax);

	// Display the windows
	int flashes = MIN_FLASHES + rand() % MAX_FLASHES;
	for (int i = 0; i < flashes; ++i) {
		int on = FLASH_ON_MIN + (rand() % (FLASH_ON_MAX));
		int off = FLASH_OFF_MIN + (rand() % (FLASH_OFF_MAX));
		redrawwin(bolt);
		wrefresh(bolt);
		usleep(on);
		redrawwin(blank);
		wrefresh(blank);
		usleep(off);
	}

	// Wait for input
	wtimeout(blank, INPUT_TIMEOUT);
	wgetch(blank);

	// Clean up
	delwin(bolt);
	delwin(blank);

	// Exit curses mode
	endwin();

	// Hide the memory
	for (int i = 0; i < xmax; ++i) {
		free(canvas[i]);
		free(resistance[i]);
		free(sky[i]);
	}
	free(canvas);
	free(resistance);
	free(sky);

	return 0;
}
