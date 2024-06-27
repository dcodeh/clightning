#include <stdio.h>
#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

static const char lightning_chars[] = {
	'_',
	'\\',
	'/',
	'|',
	'A',
	'Y',
	'%',
	'+',
	'=',
	'-',
	'X',
	'Z',
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

	char canvas[x][y];
	memset(canvas, ' ', x * y);
	srand(time(NULL));

	// generate a cool looking lightning bolt of a certain length
	int chars = rand() % (x * 2);

	// pick a random spot to start in the middle 50% of the window
	int x0 = (x / 4) + (rand() % (x / 2));
	int y0 = (y / 4) + (rand() % (y / 2));

	for (int i = 0; i < chars; ++i) {
		char c = rndchar();
		canvas[x0][y0] = c;

		switch (c) {
			case '=':
			case '-':
			case '_':
				x0++;
				break;
			case '%':
			case '\\':
				x0++;
				y0++;
				break;
			case 'Y':
			case 'X':
			case 'Z':
			case 'A':
			case '/':
				x0--;
				y0++;
				break;
			case '+':
			case '|':
				y0++;
				break;
			default:
				printf("what\n");
				exit(-1);
				break;
		}
	}

	// Output the contents of the array
	// Go home (you're drunk)
	move(0, 0);
	attron(A_BOLD);

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
			addch(canvas[i][j]);
		}
	}

	refresh();
	getch();

	// Exit curses mode
	endwin();

	return 0;
}
