#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#define TIMEOUT_NEVER -1
#define ARG_LEN_MAX 12
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

struct options {
	bool nocolor;
	bool noglow;
	bool storm;
	bool noflash;
};

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

void bolt_to_window(WINDOW *w, char **canvas, int **sky, int xmax, int ymax, const struct options *opts) {
	bool colorful = !opts->nocolor;
	for (int i = 0; i < xmax; ++i) {
		for (int j = 0; j < ymax; ++j) {
			char c = canvas[i][j];
			unsigned attribute = 0;
			if (c != ' ') {
				attribute = A_BOLD;
				if (colorful) {
					attribute |= COLOR_PAIR(BOLT_PAIR);
				}
			} else if (!opts->noglow) {
				int brightness = sky[i][j];
				attribute = A_DIM;
				if (brightness > INTENSE_THRESHOLD) {
					c = INTENSE_CHAR;
					if (colorful) {
						attribute |= COLOR_PAIR(INTENSE_GLOW_PAIR);
					}
				} else if (brightness >= MEDIUM_THRESHOLD) {
					c = MEDIUM_CHAR;
					if (colorful) {
						attribute |= COLOR_PAIR(MEDIUM_GLOW_PAIR);
					}
				} else if (brightness >= LOW_THRESHOLD) {
					c = LOW_CHAR;
					if (colorful) {
						attribute |= COLOR_PAIR(LOW_GLOW_PAIR);
					}
				} else {
					c = ' ';
					if (colorful) {
						attribute |= COLOR_PAIR(NO_GLOW_PAIR);
					}
				}
			}

			w_put_char(w, i, j, c, attribute);
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

void print_usage(void) {
	printf("Usage: ./clightning [options]\n"
			"\nDisplay lightning bolts in your terminal.\n"
			"\t--nocolor: disable color output\n"
			"\t--noglow:  disable lightning bolt glow\n"
			"\t--storm:   generate bolts until terminated; q to quit\n"
			"\t--noflash: lightning bolts don't flicker\n");
}

int parse_args(int argc, char **argv, struct options *opts) {
	if (!opts) {
		// bad programmer
		return -1;
	}

	int unknown_args = 0;
	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "--nocolor", ARG_LEN_MAX) == 0) {
			opts->nocolor = true;
		} else if (strncmp(argv[i], "--noglow", ARG_LEN_MAX) == 0) {
			opts->noglow = true;
		} else if (strncmp(argv[i], "--storm", ARG_LEN_MAX) == 0) {
			opts->storm = true;
		} else if (strncmp(argv[i], "--noflash", ARG_LEN_MAX) == 0) {
			opts->noflash = true;
		} else {
			unknown_args++;
			printf("Unsupported option: %s\n", argv[i]);
		}
	}

	return unknown_args;
}

int main(int argc, char **argv) {
	struct options opts = { 0 };
	if (parse_args(argc, argv, &opts)) {
		print_usage();
		return EXIT_FAILURE;
	}

	// Enter curses mode
	initscr();

	// Disable character buffering
	raw();

	// Disable echoing input characters that could mess up the artwork
	noecho();

	// Initialize color pairs, if supported by this terminal
	bool colorful = has_colors() && !opts.nocolor;
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

	int key = 0;
	do {
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
		bolt = newwin(ymax, xmax, 0, /* starty */ 0 /* startx */);
		wclear(bolt);

		// Write lightning bolt to window
		bolt_to_window(bolt, canvas, sky, xmax, ymax, &opts);

		// Display the windows
		if (opts.noflash) {
			wtimeout(bolt, TIMEOUT_NEVER);
			redrawwin(bolt);
			wrefresh(bolt);
			key = wgetch(bolt);
		} else {
			WINDOW *blank;
			blank = newwin(ymax, xmax, 0, /* starty */ 0 /* startx */);
			wtimeout(blank, INPUT_TIMEOUT);
			wclear(blank);
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
			key = wgetch(blank);
			delwin(blank);
		}

		// Clean up
		delwin(bolt);

		// Hide the memory
		for (int i = 0; i < xmax; ++i) {
			free(canvas[i]);
			free(resistance[i]);
			free(sky[i]);
		}
		free(canvas);
		free(resistance);
		free(sky);
	} while (opts.storm && key != 'q');

	// Exit curses mode
	endwin();

	return 0;
}
