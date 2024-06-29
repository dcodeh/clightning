CC=gcc
LIBS=-lncurses

clightning: clightning.o
	$(CC) -ggdb $< $(LIBS) -o $@

clightning.o: clightning.c
	$(CC) -c $<

clean:
	-rm clightning.o

real_clean: clean
	-rm clightning


