// Implementation file for string flex array

#include <string.h>
#include "flex.h"
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_SIZE 10

struct flex {
	int	len;		// # slots in use
	int alloc;		// allocated number of slots
	String* slot;	// array of strings
};

// -------------------------------------------------------------------------
// private function
// -------------------------------------------------------------------------
// Grow the flex array.
// Postcondition:  Slot vector contains at least one empty slot.
static void grow( Flex fa )
{
	if ( fa->alloc == 0 ) {
		fa->alloc = INITIAL_SIZE;
		fa->slot = malloc( fa->alloc * sizeof(String) );
	}
	else {
		fa->alloc *= 2;					// double its size
		fa->slot = realloc( fa->slot, fa->alloc * sizeof( String ));
	}
}

// Compare two strings pointers.
// Used by qsort().
int cmpWords ( const void* a, const void* b) {
	String aa = *(String*)a;
	String bb = *(String*)b;
	return strcmp( aa, bb );
}

// -------------------------------------------------------------------------
// public functions
// -------------------------------------------------------------------------
// Create new empty flex structure.
Flex newFlex( void )
{
	Flex fa = malloc( sizeof(*fa) );
	fa->len = 0;
	fa->alloc = 0;
	fa->slot = NULL;
	return fa;
}
			
// -------------------------------------------------------------------------
// Free flex array structure.
// Precondition:  must be empty (use extractFlex() before calling freeFlex()).
void freeFlex( Flex fa )
{
	int k;
	int length = lenFlex(fa);
	for(k = 0; k < length; k++)
		free(fa->slot[k]);
	free(fa->slot);
	free( fa );
}

// -------------------------------------------------------------------------
// Insert word into flex array.
void insertFlex( Flex fa, String word )
{
	// make sure there is room for it
	if ( fa->len == fa->alloc ) grow( fa );

	// put it into next slot and adjust length
	fa->slot[ fa->len++ ] = word;
}

// -------------------------------------------------------------------------
// Extract slot vector from flex array structure.

// Hands responsibility for array back to caller
// and empties out flex array structure.
// Note:  Call lenFlex() first to get length of slot vector.

String* extractFlex( Flex fa )
{
	return fa->slot;
}

// -------------------------------------------------------------------------
// Return length of flex array.
int lenFlex( Flex fa )
{
	return fa->len;
}

// -------------------------------------------------------------------------
// Sort flex array slot vector.
void sortFlex( Flex fa )
{
	// sort words using standard qsort function
	qsort( fa->slot, fa->len, sizeof(void*), cmpWords );
}
