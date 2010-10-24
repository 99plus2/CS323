#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include "flex.h"

int main(int argc, char* argv[])
{
	
	Flex cart = newFlex();
	char* poop = "popokpokpok";
	char* pee = "awefawefawef";
	insertFlex(cart,poop);
	insertFlex(cart,pee);
	String* wordList = extractFlex(cart);
	int length = lenFlex(cart);
	for(int i = 0; i < length; i++)
		printf("%s\n",wordList[i]);
	freeFlex(cart);

	return 0;
}
