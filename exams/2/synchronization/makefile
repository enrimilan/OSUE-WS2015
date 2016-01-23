#@file makefile
#@author Enri Miho - 0929003

all: server client

server: server.o
	gcc -o $@ $^ -lrt -pthread

client: client.o
	gcc -o $@ $^ -lrt -pthread

%.o: %.c
	gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -DENDEBUG -D_BSD_SOURCE -c -o $@ $^

clean:
	rm -f server client
	rm -f server.o client.o
