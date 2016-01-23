#@file makefile
#@author Enri Miho - 0929003

all: client server

client: client.o
	gcc -o $@ $^

server: server.o
	gcc -o $@ $^

%.o: %.c
	gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -DENDEBUG -D_BSD_SOURCE -c -o $@ $^

clean:
	rm -f client server
	rm -f client.o server.o