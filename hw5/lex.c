/*
 * Author: Douglas von Kohorn
 * Professor: Stanley Eisenstat
 * Class: CS323
 * Contents: Tokenizing implementation for Lex
 *
 */
#include "./dependencies/lex.h"
#include <malloc.h>
#include <string.h>
#include <ctype.h>

//tokenizes the next chunk of the line and updates the token with correct status
//returns a pointer to the string for the given token pointer
char* getSequence(int* origIndex, const char* line, char doubleDelimiters[], token* tok)
{
	//copy the starting index for use further down
	int index = (*origIndex);

	//go until you hit a non-simple character or non-printing character
	for(;isgraph(line[index]) && !(strchr(METACHARS,line[index]));index++) ;
	
	//if the index wasn't incremented, then check for a non-simple character
	char* nonSimple = NULL;
	if(index == (*origIndex)) nonSimple = strchr(METACHARS,line[index]);
	int redundant = 0;

	//if the character to the right of the non-simple is the same, and it's a double delimiter
	//then set redundant to true so the correct switch statement is used
	if(nonSimple && line[index] == line[index+1]
		&& strchr(doubleDelimiters,line[index+1]))
		redundant = 1;

	//switch statement for individual non-simple characters
	if(nonSimple && !redundant)
	{
		index++;
		if(*nonSimple == '<') tok->type = REDIR_IN;
		else if(*nonSimple == '|') tok->type = REDIR_PIPE;
		else if(*nonSimple == '>') tok->type = REDIR_OUT;
		else if(*nonSimple == ';') tok->type = SEP_END;
		else if(*nonSimple == '&') tok->type = SEP_BG;
		else if(*nonSimple == '(') tok->type = PAREN_LEFT;
		else if(*nonSimple == ')') tok->type = PAREN_RIGHT;
	//switch statement for double non-simple characters
	} else if (redundant)
	{
		index+=2;
		if(*nonSimple == '|') tok->type = SEP_OR;
		else if(*nonSimple == '&') tok->type = SEP_AND;
		else if(*nonSimple == '>') tok->type = REDIR_APP;
		else if(*nonSimple == '<') tok->type = REDIR_HERE;
	} else tok->type = SIMPLE;
	
	//malloc a string that contains the token
	char* text = malloc((index-(*origIndex)+1)*sizeof(char));
	strncpy(text,line+(*origIndex),index-(*origIndex));
	text[index-(*origIndex)] = '\0';

	//update the index pointer
	(*origIndex) = index;

	return text;
}

//creates a headless list of tokens based on a given line
token* lex(const char* line)
{
	//simple case for line. create a double delimiter list
	//for use in the tokenizer
	if(strlen(line) == 0) return NULL;
	char doubleDelimiters[] = "<>|&";

	//initialize index to 0, then find the first printing character
	int index = 0;
	for(;!isgraph(line[index]) && line[index] != '\0';index++) ;
	
	//logic for creating the headless list, origTok is the beginning
	//prevTok updates next throughout the execution
	token* origTok = NULL;
	token* prevTok = NULL;
	while(index < strlen(line))
	{
		//malloc token, tokenize it and set type correctly
		token* tok = malloc(sizeof(token));
		tok->text = getSequence(&index,line,doubleDelimiters,tok);

		//if prevTok != NULL, update it's next token to tok, then set
		//prevTok to tok. Otherwise, initialize origTok and prevTok
		if(prevTok) prevTok->next = tok;
		else origTok = prevTok = tok;
		prevTok = tok;

		//go to next printing character, if any
		for(;!isgraph(line[index]) && line[index] != '\0';index++) ;
	}
	if(origTok != NULL) prevTok->next = NULL;

	return origTok;
}
