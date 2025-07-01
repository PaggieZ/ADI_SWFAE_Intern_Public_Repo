CC = gcc
CFLAGS = -Wall -g

all: main

main: main.o add.o
	$(CC) $(CFLAGS) -o main main.o add.o

main.o: main.c add.h
	$(CC) $(CFLAGS) -c main.c

add.o: add.c add.h
	$(CC) $(CFLAGS) -c add.c

run: main
	./main

clean:
	rm -rf *.o main
