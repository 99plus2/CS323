#
# Author: Doug von Kohorn
# Class: CS323
# Professor: Stanley Eisenstat
#

CC=gcc
CFLAGS=-Wall -g -pedantic -std=c99

all: LEX
Lex: LEX

LEX:
	$(CC) $(CFLAGS) -o Lex ./dependencies/mainLex.c ./dependencies/getLine.c history.c lex.c

clean:
	rm -f Lex
