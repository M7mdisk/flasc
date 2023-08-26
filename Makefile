CC=gcc
CFLAGS=-Wall

all: flasc

flasc: user.c server.c flasc.c
	$(CC) $(CFLAGS) user.c server.c flasc.c -o flasc

debug: user.c server.c flasc.c
	$(CC) $(CFLAGS) -g user.c server.c flasc.c -o flasc

build: server.c flasc.c
	$(CC) -O -c server.c -o server.o
	$(CC) -O -c flasc.c -o flasco.o
	ld -r server.o flasco.o -o flasc.o
	rm flasco.o server.o

clean:
	rm *.o flasc