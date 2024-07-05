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
	'A',
};

size_t num_chars = sizeof(lightning_chars) / sizeof(char); // TODO DCB marcro for this?

char rndchar() {
	int r = rand() % num_chars;
	return lightning_chars[r];
}

void bolt(char **canvas, int x, int y, int chars, int x0, int y0) {
	printf("Start: %d, %d\n", x0, y0);
	while (chars > 0) {
		printf("%d chars remaining\n", chars);

		char c = rndchar();

		if (c == 'A') {
			// branch!
			canvas[x0][y0] = c;
			canvas[x0 - 1][y0 + 1] = '/';
			canvas[x0 + 2][y0 + 1] = '\\';
			bolt(canvas, x, y, chars / 2, x0 - 2, y0 + 2);
			bolt(canvas, x, y, chars / 2, x0 + 2, y0 + 2);
			return;
		}

		int len = (rand() % 10);

		if (len > chars) {
			len = chars;
		}

		printf("%c x %d\n", c, len);

		chars = chars - len;
		while(len--) {
			if (x0 >= 0 && y0 >= 0 && y0 < y && x0 < x) {
				canvas[x0][y0] = c;
				printf(" -> %c at %d, %d\n", c, x0, y0);
			} else {
				printf("EEK!\n");
				break;
			}

			switch (c) {
				case '-':
					x0++;
					break;
				case '\\':
					x0++;
					y0++;
					break;
				case '/':
					x0--;
					y0++;
					break;
				case '|':
					y0++;
					break;
				default:
					printf("what\n");
					exit(-1);
					break;
			}
		}
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

	// TODO DCB clean up this hacky mess
	char **canvas = malloc(x * sizeof(char *));
	for (int i = 0; i < x; ++i) {
		canvas[i] = malloc(y * sizeof(char));
		memset(canvas[i], ' ', y);
	}

	srand(time(NULL));

	// generate a cool looking lightning bolt of a certain length
	int chars = rand() % (x + y / 2);

	// TODO DCB pick a random spot to start in the middle 50% of the window
	int x0 = (x / 4) + (rand() % (x / 2));
	int y0 = (y / 4) + (rand() % (y / 2));

	bolt(canvas, x, y, chars, x0, y0);

	// Output the contents of the array
	// Go home (you're drunk)
	move(0, 0);
	attron(A_BOLD);

	for (int i = 0; i < x; ++i) {
		for (int j = 0; j < y; ++j) {
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
	}
	free(canvas);

	return 0;
}
