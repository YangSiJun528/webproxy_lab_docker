CC = gcc
CFLAGS = -O0 -Wall -Wextra -g
LDFLAGS =
HOST ?= 127.0.0.1
PORT ?= 9000

.PHONY: all clean server-run client-run

all: server client

server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

server-run: server
	./server $(PORT)

client-run: client
	./client $(HOST) $(PORT)

clean:
	rm -f server client *.o *~
