CC=gcc
CFLAGS=-Wall
DIRS = $(wildcard examples/*/)

.PHONY: all

all: flasc

flasc: user.c flasc.c core.c utils.c
	$(CC) $(CFLAGS) user.c flasc.c core.c -o flasc

debug: user.c flasc.c core.c utils.c
	$(CC) $(CFLAGS) -g user.c flasc.c core.c -o flasc

build: flasc.c core.c utils.c
	$(CC) -O -c flasc.c -o server.o
	$(CC) -O -c core.c -o flasco.o
	ld -r server.o flasco.o -o flasc.o
	rm flasco.o server.o

clean:
	rm -f *.o flasc *.out
	@for dir in $(DIRS); do \
        rm -f $${dir}main;\
    done

examples: build
	@for dir in $(DIRS); do \
		$(CC) -I. $${dir}main.c flasc.o -o $${dir}main; \
    done