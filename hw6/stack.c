#include "stack.h"

#define INITIAL_STACK_SIZE 10

// -------------------------------------------------------------------------
// definition of struct stack
struct stack {
	char** elements;
	int cursize;
	int maxsize;
};

// -------------------------------------------------------------------------
Stack newStack(void) {
	Stack s = malloc( sizeof(struct stack) );
	s->elements = malloc( INITIAL_STACK_SIZE * sizeof(char*) );
	memset(s->elements,0,INITIAL_STACK_SIZE * sizeof(char*));
	s->cursize = 0;
	s->maxsize = INITIAL_STACK_SIZE;
	return s;
}

// -------------------------------------------------------------------------
void freeStack(Stack s) {
	int i;
	for(i = 0; i < s->cursize; i++)
		if(s->elements[i]) free(s->elements[i]);
	free(s->elements);
	free( s );
}

// -------------------------------------------------------------------------
int isEmptyStack( Stack s )
{
	return ( s->cursize == 0 );
}

// -------------------------------------------------------------------------
void pushStack(Stack s, char* x) {
	if (s->cursize == s->maxsize) {
		s->maxsize *= 2;
		s->elements = realloc( s->elements, s->maxsize * sizeof(char*) );
	}
	s->elements[ s->cursize++ ] = x;
	int i;
	for(i = s->cursize; i < s->maxsize; i++)
		s->elements[i] = NULL;
}

// -------------------------------------------------------------------------
char* popStack(Stack s) {
	return s->elements[ --s->cursize ];
}

// -------------------------------------------------------------------------
void printStack(Stack s) {
	int k;
	for (k=0; k < s->cursize; k++) {
		printf( " %s", s->elements[ k ] );
	}
	printf("\n");
}

