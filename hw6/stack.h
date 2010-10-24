#ifndef _STACK_H
#define _STACK_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef
struct stack* Stack;

/* prototypes */
Stack newStack( void );
void  freeStack( Stack );
int   isEmptyStack( Stack );
void  pushStack( Stack, char* );
char* popStack( Stack );
void  printStack( Stack );

#endif
