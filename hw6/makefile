#
# Author: Doug von Kohorn
# Class: CS323
# Professor: Stanley Eisenstat
#

CC=gcc
CFLAGS=-Wall -pedantic -std=c99

all: CSH
Csh: CSH

CSH:
	$(CC) $(CFLAGS) -o Csh process.c /c/cs323/Hwk6/mainCsh.o /c/cs323/Hwk5/getLine.o /c/cs323/Hwk6/tokenize.o /c/cs323/Hwk6/parse.o /c/cs323/Hwk6/public.o stack.c

clean: rm -f Csh
