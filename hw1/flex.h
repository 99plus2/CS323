// Michael J. Fischer
// CPSC 223b, Spring 2008
// Problem set 4: wordfreq

// Header file for string flex array

#ifndef FLEX_H_
#define FLEX_H_

// type definitions
typedef struct flex* Flex;					// flex array of strings
typedef char* String;						// a null-terminated C string

// prototypes
Flex	newFlex( void );					// create new flex array
void	freeFlex( Flex fa );				// free flex array (must be empty)

int     lenFlex( Flex fa );					// length of flex array
String*	extractFlex( Flex fa );				// tear down flex array

void	insertFlex( Flex fa, String word );	// put word into flex array
void 	sortFlex( Flex fa );				// sort flex array

#endif /*FLEX_H_*/
