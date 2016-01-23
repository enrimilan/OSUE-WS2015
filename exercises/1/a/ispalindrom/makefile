#@file makefile
#@author Enri Miho - 0929003

all: ispalindrom

client: ispalindrom.o
	gcc -o $@ $^

%.o: %.c
	gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -c -o $@ $^

clean:
	rm -f ispalindrom
	rm -f ispalindrom.o