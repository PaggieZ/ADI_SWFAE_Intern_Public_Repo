CC = gcc
CFLAGS = -Wall -g

all: main

main: main.o
	$(CC) $(CFLAGS) -o main main.o

run: main
	./main

clean:
	rm -rf *.o main