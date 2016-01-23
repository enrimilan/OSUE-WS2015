#@file makefile
#@author Enri Miho - 0929003

all: forksort docs

forksort: forksort.o
	gcc -o $@ $^
docs:
	doxygen ../doc/Doxyfile

%.o: %.c
	gcc -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -c -o $@ $^

clean:
	rm -f forksort
	rm -f forksort.o
	rm -rf html
	rm -rf latex
