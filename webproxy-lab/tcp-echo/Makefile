CC = gcc
CFLAGS = -O0 -Wall -Wextra -g
CLIENT_CFLAGS = $(CFLAGS) -Dopen_listenfd=tcp_echo_unused_open_listenfd
LDFLAGS =
LIB = -lpthread
HOST ?= 127.0.0.1
PORT ?= 9000

.PHONY: all clean server-run client-run

all: server client

server: server.o csapp.o
	$(CC) $(CFLAGS) -o server server.o csapp.o $(LDFLAGS) $(LIB)

client: client.o csapp.o
	$(CC) $(CFLAGS) -o client client.o csapp.o $(LDFLAGS) $(LIB)

server.o: server.c csapp.h
	$(CC) $(CFLAGS) -c server.c

client.o: client.c csapp.h
	$(CC) $(CLIENT_CFLAGS) -c client.c

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

server-run: server
	./server $(PORT)

client-run: client
	./client $(HOST) $(PORT)

clean:
	rm -f server client *.o *~
