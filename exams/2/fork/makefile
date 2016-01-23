#@file makefile
#@author Enri Miho - 0929003

all: encode count

encode: encode.o
	gcc -o $@ $^

count: count.o
	gcc -o $@ $^

%.o: %.c
	gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -DENDEBUG -D_BSD_SOURCE -c -o $@ $^

clean:
	rm -f encode count
	rm -f encode.o count.o
