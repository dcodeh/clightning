CC=gcc
CDEVFLAGS=-Wall -Wextra -pedantic
LIBS=-lncurses

clightning: clightning.o
	$(CC) $< $(LIBS) -o $@

clightning.o: clightning.c
	$(CC) -c $<

clean:
	-rm clightning.o

real_clean: clean
	-rm clightning


