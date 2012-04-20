CC=gcc
CFLAGS = -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g -c

all: server client

server: server.o
	$(CC) -o server server.o 

client: client.o
	$(CC) -o client client.o

server.o: server.c
	$(CC) $(CFLAGS) server.c

client.o: client.c
	$(CC) $(CFLAGS) client.c

clean:
	rm -f *.o server client

.PHONY: all clean
