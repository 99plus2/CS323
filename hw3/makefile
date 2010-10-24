#
# Author: Doug von Kohorn
# Class: CS323
# Professor: Stanley Eisenstat
#

CC=gcc
CFLAGS=-Wall -g -pedantic -std=c99

all: ENCODE DECODE 
encode: ENCODE
decode: DECODE

LZW:
	make encode
	make decode

ENCODE: 
	$(CC) $(CFLAGS) -o LZW lzw.c code.c
	ln ./LZW encode
DECODE:
	ln ./LZW decode

clean:
	rm -f LZW encode decode *.o
