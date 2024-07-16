CC=gcc
LIBS=-lncurses

clightning: clightning.o
	$(CC) -g $< $(LIBS) -o $@

clightning.o: clightning.c
	$(CC) -g -c $<

clean:
	-rm clightning.o

real_clean: clean
	-rm clightning


