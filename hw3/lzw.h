/*
 * Author: Doug von Kohorn '11
 * Class: CS323
 * Professor: Stanley Eisenstat
 *
 */
#ifndef _LZW_H
#define _STACK_H

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "code.h"
#include <stdbool.h>

#define nonlethal(s, ...) fprintf(stderr,"LZW: " s ".\n", __VA_ARGS__)
#define lethal(s, ...) fprintf(stderr,"LZW: " s ".\n", __VA_ARGS__),exit(1)
#define REDUCE(x,s) ((x) & ((1 << s) - 1))
#define HASH1(p,c,s) REDUCE(((p << CHAR_BIT) | c),s)
#define HASH2(p,c,s) REDUCE((64 * HASH1(p,c,s) + 1),s)

//EMPTY is the end of the line for any entry in the hash table
//TOOBIG is the original state of prefix for every
//(pref,char) pair. Reset is used for the RESET value.
#define RESET ((1 << 20)+3)
#define EMPTY ((1 << 20)+2)
#define TOOBIG ((1 << 20)+1)

#endif
