/*
 * Author: Douglas von Kohorn
 * Professor: Stanley Eisenstat
 * Class: CS323
 * Contents: History implementation for Lex
 *
 */
#include "./dependencies/history.h"
#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

//definitions for the possible operations on an expand token
#define LAST_CMD 1
#define N_CMD 2
#define N_PREV 3
#define SEARCH 4
#define NORMAL 5
#define ASTERISK 6
#define CARAT 7
#define COLON 8
#define DOLLAR 9

//memory bank for the command containing an array of token* and
//their corresponding command number, the 324 spot in the array
//is reserved for out of bounds logic in expandTok()
typedef struct memory
{
	int cur;
	token** tokens;
	int commandNum[324];
} Memory;

//initializes the memory
Memory* createMemory()
{
	Memory* temp = malloc(sizeof(Memory));
	memset(temp->commandNum,0,324*sizeof(int));
	temp->cur = 0;
	temp->tokens = malloc(323*sizeof(token*));
	memset(temp->tokens,0,323*sizeof(token*));
	return temp;
}

//the memory is a global variable so all functions can access it at any time
//and the memory remains invisible to the execution of the program
static Memory* commands = NULL;

//a string replacer method for replacing the memory commands with
//the right string. replaces orig with rep in str, then sets index correctly
char *replace_str(char *str, char *orig, char *rep, int* index)
{
	//update the index to reflect the correct position after replacement
	*index += strlen(rep)-strlen(orig);

	//calculate the string size post-replacement
	int newSize = strlen(str) - strlen(orig) + strlen(rep) + 1;
	char* buffer = malloc(newSize*sizeof(char));

	//find where the substring begins
	char* p = strstr(str,orig);
	
	//copy the first part of the string to the buffer
	strncpy(buffer, str, p-str);
	buffer[p-str] = '\0';

	//copy the second part of the string to the buffer
	sprintf(buffer+(p-str), "%s%s", rep, p+strlen(orig));
	buffer[newSize-1]= '\0';

	free(rep);
	free(orig);
	free(str);
	return buffer;
}

//simple substring search function, returns cmd if found, 323 otherwise
int searchText(token* tok, char* string, int cmd)
{
	for(;tok!=NULL;tok=tok->next)
		if(strstr(tok->text,string)) return cmd;
	return 323;
}

//finds the correct token, sets status to 1, and returns it, if possible
//if not possible, sets status to -1, returns NULL
//increments index to the character AFTER the last important value
token* getToken(int operation, int* status, char* line, int* index)
{
	//pEnd is for finding the correct index AFTER an int is read
	char* pEnd;
	long int tokenIndex = 323;
	
	//prevCmd is the proper previous command index (if there is one)
	int prevCmd = commands->cur-1;
	prevCmd += (prevCmd < 0) ? 323 : 0;

	//returns token in prevCmd, if possible, increments index accordingly
	if(operation == LAST_CMD)
	{
		tokenIndex = prevCmd;
		*index += 2;
	//returns the command prevCmd - X + 1, where X is from !X
	} else if(operation == N_PREV)
	{
		//convert X to long int, make sure it's within bounds
		int nPrev = strtol(line+(*index)+2,&pEnd,10);
		if(nPrev == 0 || nPrev > 323) tokenIndex = 323;
		else
		{
			tokenIndex = prevCmd - nPrev + 1;
			tokenIndex += (tokenIndex < 0) ? 323 : 0;
		}

		//increment index accordingly
		*index += pEnd - (line+(*index));
	//similar to N_PREV, but returns command X
	} else if(operation == N_CMD)
	{
		tokenIndex = strtol(line+(*index)+1,&pEnd,10);

		//0, an index greater than previous command, and and index
		//farther than 323 away from prevCmd are unacceptable
		if(tokenIndex == 0 || tokenIndex > commands->commandNum[prevCmd]
		   || tokenIndex <= (commands->commandNum[prevCmd] - 323)) tokenIndex = 323;
		else tokenIndex = (tokenIndex - 1) % 323;

		*index += pEnd - (line+(*index));
	} else if(operation == SEARCH)
	{
		//find the length of the string, create the search string
		int i, numBytes = strcspn(line+(*index)+2,"?")+1;
		char searchString[numBytes];
		strncpy(searchString,line+(*index)+2,numBytes);
		searchString[numBytes-1] = '\0';

		//increment index accordingly
		*index += 2+numBytes;

		//search for token
		for(i=0;i<323;i++)
		{
			int location = prevCmd - i;
			location += (location < 0) ? 323 : 0;
			
			//set tokenIndex to prevCmd if found, 323 (out-of-range => status is set to -1)
			//exit as soon as first matching token is found
			tokenIndex = searchText(commands->tokens[location],searchString,location);
			if(tokenIndex != 323) break;
		}
	}

	//if command not listed, set status correctly, then return correct value
	if(tokenIndex == 323 || commands->commandNum[tokenIndex] <= 0) *status = -1;
	if(*status != -1) *status = 1;
	return (commands->commandNum[tokenIndex] > 0)
		? commands->tokens[tokenIndex] : NULL;
}

//returns a copy of text if copy == 1, otherwise
//returns whitespace and sets status to -1
char* getText(char* text, int copy, int* status)
{
	int i;
	char* string;
	if(!copy)
	{
		string = malloc(1*sizeof(char));
		string[0] = '\0';
		*status = -1;
	} else
	{
		string = malloc((strlen(text)+1)*sizeof(char));
		for(i=0;i<strlen(text);i++) string[i] = text[i];
		string[i] = '\0';
	}
	return string;
}

//function to create a string containing the correct token(s) from
//tok, and set status to 1 if worked, -1 otherwise
char* getString(token* tok, int modifier, int number, int* status)
{
	char* string = NULL;
	token* p = tok;
	int i, accum;

	//otherwise if NORMAL or ASTERISK, create string from all tokens
	if(modifier <= ASTERISK && p != NULL)
	{
		//special case: token[1] == NULL, csh returns the null string, not failure
		if(modifier == ASTERISK)
		{
			p = p->next;
			if(p == NULL) return getText(NULL,0,&number);
		}

		//find the length (with space in between tokens)
		for(accum = 0; p != NULL; p=p->next)
			accum += strlen(p->text)+1;
		string = malloc((accum)*sizeof(char));
		
		if(modifier == ASTERISK) p = tok->next;
		else p = tok;

		//string tokens together
		for(accum=0; p!=NULL; p=p->next)
		{
			for(i=0; i<strlen(p->text); i++) string[accum+i] = p->text[i];
			string[accum+i] = ' ';
			accum += i+1;
		}
		string[accum-1] = '\0';
		p = tok;
	} else if(modifier == DOLLAR && p != NULL)
	{
		//go to last token, set string to its text
		for(; p->next != NULL; p=p->next) ;
		string = getText(p->text,1,status);
	//COLON is like CARAT with number set to 1
	} else if((modifier == COLON || modifier == CARAT) && p != NULL)
	{
		if(modifier == CARAT) number = 1;

		//go to number's text, if possible, set string to its text
		for(i=0;i<number;i++)
			if(p != NULL) p=p->next;
		if(p != NULL) string = getText(p->text,1,status);
	} 

	//if p gets set to null, then set string to whitespace, status to -1
	if(p == NULL) string = getText(NULL,0,status);
	if(*status != -1) *status = 1;
	return string;
}

//expands all the memory commands into the correct tokens, if possible
//sets status to correct number if not possible
char* hExpand(const char* oldLine, int* status)
{
	//operation is the correct expand operation, modifier and number help
	//getString return the correct text
	int operation = NORMAL,modifier = NORMAL,number = 0;
	token* expandTok = NULL;

	//special case, memory not initialized, set status to 0 initially
	*status = 0;
	if(commands == NULL) commands = createMemory();

	//read until the first newline, copy that part of oldLine into newLine
	char* newLine = malloc((strlen(oldLine)+1)*sizeof(char));
	strcpy(newLine,oldLine);

	//get index of first '!', offset remembers where index was BEFORE increment
	//if no '!', return unadulterated line
	int index = strcspn(newLine,"!");
	int offset = index;
	if(index >= strlen(newLine)) return newLine;

	//main loop for expansion
	while(!(index >= strlen(newLine)))
	{
		//first check for operator type, if any recognizable
		operation = modifier = NORMAL;

		if(newLine[index+1] == '!') operation = LAST_CMD;
		else if(isdigit(newLine[index+1])) operation = N_CMD;
		else if(newLine[index+1] == '-'
			&& isdigit(newLine[index+2])) operation = N_PREV;
		else if(newLine[index+1] == '?' && newLine[index+2] != '?' && 
			strcspn(newLine+index+2,"!") > strcspn(newLine+index+2,"?")) //no ! allowed
		{
			//no whitespace allowed in STRING
			int isSpace = 0, i;
			for(i=index+2; i<=strcspn(newLine+index+2,"?")+index+2; i++) 
				if(isspace(newLine[i])) isSpace = 1;

			//if no space or ! in STRING, then allow search to commence
			if(!isSpace) operation = SEARCH;
		}

		//otherwise, skip to next !
		if(operation == NORMAL)
		{
			index = strcspn(newLine+index+1,"!")+index+1;
			continue;
		}

		//offset remembers where index was BEFORE increment
		//then expandTok gets the proper token, if possible
		offset = index;
		expandTok = getToken(operation,status,newLine,&index);

		//second, check for any modifiers for the token
		if(newLine[index] == '*') modifier = ASTERISK;
		else if(newLine[index] == '^') modifier = CARAT;
		else if(newLine[index] == '$') modifier = DOLLAR;
		else if(newLine[index] == ':' && isdigit(newLine[index+1]))
		{
			//everything but COLON is simple, this routine gets the
			//number and increments index accordingly
			char* pEnd;
			modifier = COLON;
			number = strtol(newLine+index+1,&pEnd,10);
			index += pEnd - (newLine+index);
		}

		//every modifier is one character except no modifier or COLON
		if(modifier != NORMAL && modifier != COLON) index++;

		//original is the string to be replaced
		char* original = malloc((index-offset+1)*sizeof(char));
		original[index-offset] = '\0';
		strncpy(original,newLine+offset,index-offset);

		//replacement is the string to replace original
		char* replacement = getString(expandTok,modifier,number,status);
		newLine = replace_str(newLine,original,replacement,&index);

		//set index to next !
		index = strcspn(newLine+index,"!")+index;
	}

	return newLine;
}

//same freelist method that appears in mainLex.c
void freelist (token *list)
{
	if(list == NULL) return;

	token *p, *pnext;
	for (p = list;  p;  p = pnext)
	{
		pnext = p->next;
		free(p->text);
		free(p);
	}
}

//copies a list of tokens recursively
token* tokenCopy(token* list)
{
	//the stopping condition for recursion
	if(list == NULL) return NULL;

	//malloc a new token
	token* temp = malloc(sizeof(token));
	temp->text = malloc((strlen(list->text)+1)*sizeof(char));

	//copy values to it
	strcpy(temp->text,list->text);
	temp->type = list->type;

	//recurse deeper
	temp->next = tokenCopy(list->next);

	return temp;
}

//makes a copy of list, and remembers the command number
//corresponding to it
void hRemember(int ncmd, token* list)
{
	//create memory, if necessary
	if(commands == NULL) commands = createMemory();

	//if a token exists in the current command slot, free it
	token* temp = commands->tokens[commands->cur];
	freelist(temp);

	temp = tokenCopy(list);

	//update memory
	commands->commandNum[commands->cur] = ncmd;
	commands->tokens[commands->cur] = temp;
	commands->cur++;
	if(commands->cur == 323) commands->cur = 0;
}

//creates a new memory for the shell
void hClear()
{
	if(commands == NULL) return;

	//free all associated memory, then create new memory
	int i;
	for(i=0;i<323;i++) freelist(commands->tokens[i]);
	free(commands->tokens);
	free(commands);
	commands = NULL;
}

//dumps the last n tokenized commands, in order
void hDump(int n)
{
	//logic for setting i the the correct value
	int num, i = commands->cur - n;
	struct token* p;

	//if i - n wants to include commands before 0, they must exist,
	//otherwise i = 0
	if(i<0) i += (commands->commandNum[322] > 0) ? 323 : -i;

	//go through command list, and print out tokens with %6d %s ... %s format
	for(num = 1; num <= n; i++, num++)
	{
		//make sure i is within bounds and referencing an existing token
		if(i > 322) i = 0;
		if(commands->commandNum[i] <= 0) continue;

		//print out number, print out commands
		printf("%6d ",commands->commandNum[i]);
		for(p = commands->tokens[i]; p != NULL; p = p->next) printf(" %s", p->text);
		printf("\n");
	}
}
