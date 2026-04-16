# Makefile for Proxy Lab 
#
# You may modify this file any way you like (except for the handin
# rule). You instructor will type "make" on your specific Makefile to
# build your proxy from sources.

CC = gcc
CFLAGS = -g -Wall
LDFLAGS = -lpthread
PROXY_PORT ?= 4500
TINY_PORT ?= 8000
ASAN_UBSAN_CFLAGS = -g -O1 -Wall -fsanitize=address,undefined -fno-omit-frame-pointer
ASAN_UBSAN_LDFLAGS = -lpthread -fsanitize=address,undefined
VALGRIND_CFLAGS = -g -O0 -Wall
SANITIZER_ENV = ASAN_OPTIONS=detect_leaks=1 UBSAN_OPTIONS=print_stacktrace=1

.PHONY: all clean driver-run driver-asan-ubsan proxy-run proxy-asan-ubsan proxy-valgrind tiny-run tiny-asan-ubsan tiny-valgrind

all: proxy

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h
	$(CC) $(CFLAGS) -c proxy.c

proxy: proxy.o csapp.o
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy $(LDFLAGS)

driver-run: proxy
	$(MAKE) -C tiny
	./driver.sh

driver-asan-ubsan:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(ASAN_UBSAN_CFLAGS)' LDFLAGS='$(ASAN_UBSAN_LDFLAGS)' proxy
	$(MAKE) -C tiny clean
	$(MAKE) -C tiny
	$(SANITIZER_ENV) ./driver.sh

proxy-run: proxy
	./proxy $(PROXY_PORT)

proxy-asan-ubsan:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(ASAN_UBSAN_CFLAGS)' LDFLAGS='$(ASAN_UBSAN_LDFLAGS)' proxy
	$(SANITIZER_ENV) ./proxy $(PROXY_PORT)

proxy-valgrind:
	$(MAKE) clean
	$(MAKE) CFLAGS='$(VALGRIND_CFLAGS)' proxy
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./proxy $(PROXY_PORT)

tiny-run:
	$(MAKE) -C tiny
	cd tiny && ./tiny $(TINY_PORT)

tiny-asan-ubsan:
	$(MAKE) -C tiny clean
	$(MAKE) -C tiny CFLAGS='$(ASAN_UBSAN_CFLAGS) -I .' all
	cd tiny && $(SANITIZER_ENV) ./tiny $(TINY_PORT)

tiny-valgrind:
	$(MAKE) -C tiny clean
	$(MAKE) -C tiny CFLAGS='$(VALGRIND_CFLAGS) -I .' all
	cd tiny && valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./tiny $(TINY_PORT)

# Creates a tarball in ../proxylab-handin.tar that you can then
# hand in. DO NOT MODIFY THIS!
handin:
	(make clean; cd ..; tar cvf $(USER)-proxylab-handin.tar proxylab-handout --exclude tiny --exclude nop-server.py --exclude proxy --exclude driver.sh --exclude port-for-user.pl --exclude free-port.sh --exclude ".*")

clean:
	rm -f *~ *.o proxy core *.tar *.zip *.gzip *.bzip *.gz
