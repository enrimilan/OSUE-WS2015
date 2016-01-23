#@file makefile
#@author Enri Miho - 0929003

all: battleships-client battleships-server docs

battleships-client: battleships-client.o
	gcc -o $@ $^ -lrt -pthread

battleships-server: battleships-server.o
	gcc -o $@ $^ -lrt -pthread
docs:
	doxygen ../doc/Doxyfile

%.o: %.c
	gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -DENDEBUG -D_BSD_SOURCE -c -o $@ $^

clean:
	rm -f battleships-client battleships-server
	rm -f battleships-client.o battleships-server.o
	rm -rf ../doc/html
	rm -rf ../doc/latex
