/*
 * Author: Douglas von Kohorn '11
 * Class: CS323
 * Professor: Stanley Eisenstat
 *
 */
#include "lzw.h"

//(pref,char) pair using bit fields, 8 for the char, 21 for the prefix
typedef struct group
{
	unsigned tackOn:8;
	unsigned prefix:21;
} Group;

//table with pointer to array of Group and tabulation variables
typedef struct table
{
	int maxBits,numEntries,currentBits;
	Group* entries;
} Table;

//uses double hashing to find (pref,char) pair in table
//formula = (start + i*increment) % (1 << currentBits)
unsigned int getCode(unsigned int code, unsigned char k, Table* table)
{
	unsigned int i,index,testCode;
	unsigned char testK;
	unsigned int start = HASH1(code,k,table->currentBits);
	unsigned int increment = HASH2(code,k,table->currentBits);
	for(i = 0; i < (1 << table->currentBits); i++)
	{
		index = REDUCE(start + i*increment,table->currentBits);
		testK = table->entries[index].tackOn;
		testCode = table->entries[index].prefix;
		if(testCode == TOOBIG)
			return TOOBIG;
		if((code == testCode) && (k == testK))
			return index;
	}
	return TOOBIG;
}

//adds (pref,char) pair to table, returns index of interest, uses
//same hashing formula as getCode(), increments numEntries for
//successful add.
unsigned int addCode(unsigned int code, unsigned char k, Table* table)
{
	unsigned int i,index;
	unsigned int start = HASH1(code,k,table->currentBits);
	unsigned int increment = HASH2(code,k,table->currentBits);
	for(i = 0; i < (1 << table->currentBits); i++)
	{
		index = REDUCE(start + i*increment,table->currentBits);
		if(table->entries[index].prefix == TOOBIG)
		{
			table->numEntries++;
			table->entries[index].tackOn = k;
			table->entries[index].prefix = code;
			return index;
		}
	}
	return TOOBIG;
}

//creates table with maxBits and currentBits
//and initializes 256 ASCII values and RESET value
Table* makeTable(int maxBits, int currentBits)
{
	Table* hashTable = malloc(sizeof(Table));
	hashTable->maxBits = maxBits;
	hashTable->numEntries = 0;
	hashTable->currentBits = currentBits;
	hashTable->entries = malloc((1 << currentBits)*sizeof(Group));

	int i;
	for(i = 0; i < (1 << currentBits); i++)
		hashTable->entries[i].prefix = TOOBIG;
	for(i = 0; i < 256; i++)
		addCode(EMPTY,i,hashTable);
	
	//add reset value
	addCode(RESET,0,hashTable);

	return hashTable;
}

//recursive function for resizing the hash table
unsigned int resizeHelper(unsigned int prefix, unsigned int tackOn, Table* newTable, Table* oldTable)
{
	//if (EMPTY,k) doesn't exist, add it and return the index
	//otherwise just return the index where it exists
	if(prefix == EMPTY || prefix == RESET)
	{
		unsigned int checkit = getCode(prefix,tackOn,newTable);
		return (checkit == TOOBIG) ? addCode(prefix,tackOn,newTable) : checkit;
	}

	//recurse backwards to the last prefix until hitting (EMPTY,k)
	//similar logic to above step
	unsigned int newPrefix = resizeHelper(oldTable->entries[prefix].prefix,
					      oldTable->entries[prefix].tackOn,newTable,oldTable);
	unsigned int isAdded = getCode(newPrefix,tackOn,newTable);
	
	return (isAdded == TOOBIG) ? addCode(newPrefix,tackOn,newTable) : isAdded;
}

//starts the recursive resizing of the hashTable, returns new hashTable
Table* resizeTable(Table* hashTable)
{
	int i, newBits = hashTable->currentBits;
	Table* newTable = makeTable(hashTable->maxBits,newBits); 
	unsigned int prefix,tackOn;

	//go through all old values of hashTable and rehash them
	for(i = 0; i < (1 << (newBits - 1)); i++)
	{
		prefix = hashTable->entries[i].prefix;
		tackOn = hashTable->entries[i].tackOn;
		if(prefix != TOOBIG)
			resizeHelper(prefix,tackOn,newTable,hashTable);
	}
	free(hashTable->entries);
	free(hashTable);
	return newTable;
}

//encodes the bitstream up with up to 2^maxBits entries
void encode(int maxBits, int block, double ratio)
{
	//write maxBits to stream in unary
	Table* hashTable = makeTable(maxBits,9);
	unsigned long int maxCode = ((1 << (maxBits)) - 1);
	putBits(20,maxCode);

	int tester;
	unsigned char k;
	unsigned int code = EMPTY, possCode, nRead = 0, nSent = 0;
	double load;
	bool stopIncrease = false, found = false;
	while((tester=getc(stdin)) != EOF)
	{
		nRead+=8;
		k = (unsigned char) tester;
		possCode = getCode(code,k,hashTable);

		//normal algorithm, calculate load as long as currentBits < maxBits
		load = stopIncrease ? 0 : (hashTable->numEntries + 1)/(1 << hashTable->currentBits);
		if(possCode != TOOBIG)
		{
			code = possCode;

			//found tells the BLOCK.RATIO implementation whether to
			//output code before resetting the table
			found = true;
		} else
		{
			nSent+=hashTable->currentBits;
			putBits(hashTable->currentBits,code);

			//if load will be > .99, resize only if currentBits+1 < maxBits
			if(load >= .99 && !stopIncrease)
			{
				hashTable->currentBits++;
				if(hashTable->maxBits < hashTable->currentBits)
				{
					stopIncrease = true;
					hashTable->currentBits--;
				}
				if(!stopIncrease)
					hashTable = resizeTable(hashTable);
			}

			//stop adding codes when currentBits == maxBits
			if(!stopIncrease)
				addCode(code,k,hashTable);
			code = getCode(EMPTY,k,hashTable);
		}

		//if block == 0, then -r switch not specified
		if(block != 0 && (nRead/8) >= block)
		{
			bool failure = (nSent > ((double) ratio * nRead)) ? true : false;
			if(failure)
			{
				//if previous code hasn't been output, then do so
				//then signal decode to reset its table
				if(found)
					putBits(hashTable->currentBits,code);
				putBits(hashTable->currentBits,getCode(RESET,0,hashTable));

				free(hashTable->entries);
				free(hashTable);
				hashTable = makeTable(maxBits,9);

				stopIncrease = false;

				//if last read char hasn't been accounted for, do so
				//otherwise start over with code == EMPTY
				code = (!found) ? getCode(EMPTY,k,hashTable) : EMPTY;
			}
			nRead = nSent = 0;
		}
		found = false;
	}

	if(code != EMPTY)
		putBits(hashTable->currentBits,code);

	free(hashTable->entries);
	free(hashTable);
	flushBits();
}

//recursive function for outputting stream of chars
unsigned char decodeHelper(unsigned int code, Table* hashTable, unsigned char* finalK)
{
	if(code >= (1 << hashTable->currentBits)) lethal("code %d out of range",code);

	//if reached (EMPTY,k), finalK = k and return k.
	if(hashTable->entries[code].prefix == EMPTY)
		return (*finalK) = hashTable->entries[code].tackOn;
	else
	{
		//else, recurse deeper then return current frame's char
		putc(decodeHelper(hashTable->entries[code].prefix,hashTable,finalK),stdout);
		return hashTable->entries[code].tackOn;
	}
}

//decode compressed stream
void decode()
{
	//get maxBits in unary from the stream
	int i,maxBits = 0;
	for(i = 0; i < 20; i++) maxBits += getBits(1);
	Table* hashTable = makeTable(maxBits,9);

	if(maxBits < 9 || maxBits > 20) lethal("maxbits %d out of range",maxBits);

	unsigned int oldCode = EMPTY,newCode,code;
	unsigned char finalK,tempK;
	double load;
	bool KwKwK = false,stopIncrease = false;
	while((code = newCode = getBits(hashTable->currentBits)) != EOF)
	{
		if(code >= (1 << hashTable->currentBits)) lethal("code %d out of range",code);

		//if reset code is found, then reset table
		if(code == getCode(RESET,0,hashTable))
		{
			free(hashTable->entries);
			free(hashTable);
			hashTable = makeTable(maxBits,9);

			stopIncrease = false;
			oldCode = EMPTY;
			continue;
		}

		//account for KwKwK
		if(hashTable->entries[code].prefix == TOOBIG)
		{
			tempK = finalK;
			code = oldCode;
			KwKwK = true;
		}
		
		//recurse and output string, if KwKwK then output last char
		putc(decodeHelper(code,hashTable,&finalK),stdout);
		if(KwKwK)
			putc(tempK,stdout);
		KwKwK = false;

		if(oldCode != EMPTY && !stopIncrease)
			addCode(oldCode,finalK,hashTable);

		//calculate load, increase if currentBits+1 < maxBits
		load = stopIncrease ? 0 : (hashTable->numEntries + 1)/(1 << hashTable->currentBits);
		if(load >= .99 && !stopIncrease)
		{
			hashTable->currentBits++;
			if(hashTable->maxBits < hashTable->currentBits)
			{
				stopIncrease = true;
				hashTable->currentBits--;
			}
			if(!stopIncrease)
				hashTable = resizeTable(hashTable);
		}
		oldCode = newCode;
	}

	free(hashTable->entries);
	free(hashTable);
}

//returns 1 if function == encode, 2 if function == decode
//exits on no match
int checkFunction(char* function)
{
	int returnValue = 0;
	if((strcmp(function,"encode")) && (strcmp(function,"decode")))
		lethal("encode [-m MAXBITS] [-r BLOCK.RATIO] or decode, not %s",function);
	else
	{
		if(strcmp(function,"encode") == 0)
			returnValue = 1;
		if(strcmp(function,"decode") == 0)
			returnValue = 2;
	}
	return returnValue;
}

//parses the -r switch for block and ratio, exits on <= 0
void parseR(char* period, char* argument, int* block, double* ratio)
{
	int tempBlock;
	double tempRatio;
	tempRatio = atof(period);

	//after extracting ratio, set '.' to '\0' and use atoi on block
	(*period) = '\0';
	tempBlock = atoi(argument);

	//atof doesn't like it when RATIO = '.-200', but that is okay by the spec,
	//so I treat it like a special case
	if((tempRatio == 0 && (*(period+1)) != '-' && (*(period+1)) != '0') || tempBlock <= 0)
		lethal("%s or %s is an invalid specification in argument", argument, (period+1));
	
	(*block) = tempBlock;

	//if the negative is either on the right or left of the period
	//it doesn't matter what the ratio is as long as it's negative
	if((*(period+1)) == '-' || (*(period-1)) == '-')
		(*ratio) = -1;
	else
		(*ratio) = tempRatio;
}

//parses the (switch, argument) groups of statements
int parseArguments(int argc, char* argv[], int* maxBits, double* ratio, int* block, int* action)
{
	int switchActivated = 0, returnValue = 0,i;
	char* function = strrchr(argv[0], '/');

	//first take care of argv[0]
	if(function == NULL)
		returnValue = checkFunction(argv[0]);
	else
		returnValue = checkFunction(++function);
	//then take care of everything else
	for(i = 1; i < argc; i++)
	{
		//if argument, then parse for either maxBits (switchActivated == 1)
		//or BLOCK.RATIO (switchActivated == 2), exit on various errors
		if(switchActivated)
		{
			if(switchActivated == 1)
				(*maxBits) = atoi(argv[i]);
			else if(switchActivated == 2)
			{
				char* period = strchr(argv[i],'.');
				if(period == NULL)
					lethal("invalid specification, no period in %s [-r BLOCK.RATIO]", period);
				else
					parseR(period,argv[i],block,ratio);
			}
			switchActivated = 0;
		} else
		{
			//if not -m or -r, exit, otherwise set switch accordingly
			if(strcmp(argv[i],"-m") == 0)
				switchActivated = 1;
			else if(strcmp(argv[i],"-r") == 0)
				switchActivated = 2;
			if(!switchActivated)
				lethal("invalid switch %s used",argv[i]);
		}
	}

	if(switchActivated != 0)   
		lethal("usage: encode [-m MAXBITS] [-r BLOCK.RATIO] in %d",switchActivated);

	//reduce maxBits if outside range
	if((*maxBits) > 20 || (*maxBits) < 9)
		(*maxBits) = 12;

	return returnValue;
}

//parse and operate on input 
int main(int argc, char* argv[])
{
	//maxBits == 12 default
	int maxBits = 12, block = 0, action;
	double ratio;
	action = parseArguments(argc,argv,&maxBits,&ratio,&block,&action);

	if(action == 1) encode(maxBits,block,ratio);
	if(action == 2)	decode();

	return 0;
}
