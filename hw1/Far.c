#include "flex.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <linux/limits.h>

char* createRel(char* curRelPath, char* name)
{
	//special case is curRelPath is NULL, otherwise a '/' must be wedged in between
	int lengthRelPath,j;
	if(curRelPath == NULL)
		lengthRelPath = strlen(name) + 1;
	else
		lengthRelPath = strlen(curRelPath) + strlen(name) + 2;

	char* newRelPath = malloc(lengthRelPath * sizeof(char));

	//wedge the '/' if necessary
	if(curRelPath != NULL)
	{
		for(j=0;j<lengthRelPath - 1; j++)
		{
			if(j < strlen(curRelPath))
				newRelPath[j] = curRelPath[j];
			else if(j == strlen(curRelPath))
				newRelPath[j] = '/';
			else
				newRelPath[j] = name[j-strlen(curRelPath) - 1];
		}
		newRelPath[lengthRelPath-1] = '\0';
	} else
		strcpy(newRelPath,name);
	
	return newRelPath;
}

void makeDirs(char* fileName)
{
	struct stat statBuf;

	//find where filename begins and directory path ends
	int i,length;
	for(i = strlen(fileName) - 1; i >= 0; i--)
		if(fileName[i] == '/')
			break;

	//make a directory string
	length = i+1;
	char* dir = malloc((length + 1) * sizeof(char));
	strncpy(dir,fileName,length);
	dir[length] = '\0';

	char* temp;
	for(i = 1; i < length; i++)
	{
		if(dir[i] == '/')
		{
			//if the directory exists, no need to make a directory, go down the line of dir
			temp = malloc((i+1)*sizeof(char));
			strncpy(temp,dir,i);
			temp[i] = '\0';
			int exists = lstat(temp,&statBuf);
			if(exists != 0)
			{
				if(mkdir(temp, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) != 0)
					fprintf(stderr, "Far: cannot create %s\n",fileName);
			}
			free(temp);
		}
	}
	free(dir);
}

int isDirOf(char* argv, char* fileName)
{
	//obviously, argv must be smaller than the fileName
	if(strlen(argv) >= strlen(fileName))
		return 0;

	//get length of directory string, account for trailing '/' if there
	int length = strlen(argv);
	if(argv[length - 1] == '/')
		length -= 1;

	//if argv is subpath, then the subpath length of fileName must be a '/'
	if(fileName[length] != '/')
		return 0;

	//find if an equal substring of fileName and argv match, if so it's a directoryOf
	int i;
	for(i=0;i<length;i++)
		if(argv[i] != fileName[i])
			return 0;
	return 1;
}

int checkMatch(char* argv[], char* fileName, int argc)
{
	int i;
	for(i = 3; i < argc; i++)
	{
		if(strcmp(argv[i],fileName) == 0) return i;
		if(isDirOf(argv[i],fileName)) return i;
	}
	return 0;
}

void xkey(char* infile, char* argv[], int argc)
{
	FILE* in = fopen(infile, "r");
	if(in == NULL)
	{
		fprintf(stderr,"Far: Archive %s does not exist\n", infile);
		exit(0);
	}
	FILE* extract;

//	boolArray records which of the arguments got matched, so that errors can be reported afterwards
	int boolArray[argc];
	int j;
	for(j = 0; j < argc; j++)
		boolArray[j] = 0;
	char c = fgetc(in);
	char fileName[PATH_MAX];
	int byteSize;
	while(c != EOF)
	{
		ungetc(c, in);

		//get file name
		fgets(fileName,PATH_MAX,in);
		fileName[strlen(fileName)-1] = '\0';

		//get byte size, skip over '|'
		fscanf(in,"%d",&byteSize);
		fgetc(in);
		
		//if no args specified, then match = 1 and every file is extracted, otherwise, check for match
		int i,match;
		if(argc >  3) match = checkMatch(argv,fileName,argc);
		else match = 1;

		if(match > 0)
		{
			boolArray[match] = 1;
			//open file, write contents to it, if DNE, then create necessary dirs
			extract = fopen(fileName,"w");
			if(extract == NULL) 
			{
				makeDirs(fileName);
				extract = fopen(fileName,"w");
			}
			if(extract == NULL) fprintf(stderr, "Far: cannot create %s", fileName);

			//extract
			for(i = 0; i < byteSize; i++) fputc(fgetc(in),extract);
			fclose(extract);
		} else
		{
			for(i = 0; i < byteSize; i++) fgetc(in);
		}

		c = fgetc(in);
	}
	if(argc >= 3)
		for(j = 3; j < argc; j++)
			if(boolArray[j] == 0)
				fprintf(stderr,"Far: cannot find %s\n",argv[j]);

	fclose(in);
}

void dkey(int argc, char* argv[], char* infile, int calledByRkey)
{
	FILE* in = fopen(infile, "r");
	if(in == NULL)
	{
		fprintf(stderr,"Far: archive %s does not exist\n", infile);
		exit(0);
	}

	FILE* temp = tmpfile();

//	boolArray keeps track of which arguments have been deleted, and reports those which havent
	int j;
	int boolArray[argc];
	for(j = 0; j < argc; j++)
		boolArray[j] = 0;
	char fileName[PATH_MAX];
	int byteSize;
	char c = fgetc(in);
	while(c != EOF)
	{
		ungetc(c,in);

//		get file name
		fgets(fileName,PATH_MAX,in);
		fileName[strlen(fileName)-1] = '\0';
		
//		get byte size, convert to int, skip over '|'
		fscanf(in,"%d",&byteSize);
		fgetc(in);

//		find out if file is named in the arguments
		int match = checkMatch(argv,fileName,argc);
		int i;
		
//		check for existence, if doesn't exist, keep in file by setting match to 0 (only when replacing)
		struct stat statBuf;
		int success = stat(fileName,&statBuf);
		if(success != 0 && calledByRkey)
			match = 0;
	
//		if it matches, skip over size bytes, else, copy over size bytes
		if(match > 0)
		{
			boolArray[match] = 1;
			for(i = 0; i < byteSize; i++) fgetc(in);
		} else
		{
			fprintf(temp,"%s\n",fileName);
			fprintf(temp,"%d|",byteSize);
			char tester = fgetc(in);
			for(i = 0; i < byteSize; i++,tester=fgetc(in)) fputc(tester,temp);
			ungetc(tester,in);
		}
		c = fgetc(in);
	}
//	report if deletion did not happen
	if(argc >= 3 && !calledByRkey)
		for(j = 3; j < argc; j++)
			if(boolArray[j] == 0)
				fprintf(stderr,"Far: cannot find %s\n",argv[j]);

//	copy temp file to archived file
	fclose(in);
	in = fopen(infile,"w");
	rewind(temp);
	
	char test = fgetc(temp);
	for(;test != EOF;test = fgetc(temp))
		fputc(test,in);

	fclose(in);
	fclose(temp);
}

void append(FILE* in, char* relativePath, struct stat statBuf, Flex fileList)
{
	//open all the arg files for appending to the archive
	FILE* curFile = fopen(relativePath,"r");
	if(curFile == NULL) fprintf(stderr, "file %s does not exist\n", relativePath);

	//check to see if relative Path is in fileList, if not, add
	int length = lenFlex(fileList);
	String* words = extractFlex(fileList);
	int i;
	int addIn = 1;
	for(i = 0; i < length; i++)
		if(strcmp(relativePath,words[i]) == 0)
		{
			addIn = 0;
			free(relativePath);
			break;
		}
	
	if(addIn)
	{
		//add to flex array
		insertFlex(fileList,relativePath);

		//append the name to the archive
		fprintf(in,"%s\n",relativePath);
	
		//get length and append to archive
		int length = statBuf.st_size;
		fprintf(in,"%d|",length);
	
		//reset point and append to archive
		char test = fgetc(curFile);
		for(;test != EOF; test = fgetc(curFile))
			fputc(test,in);
	}

	fclose(curFile);
}

void rkeyHelper(char* relativePath, FILE* in, Flex fileList)
{
	DIR *dp;
	struct dirent *entry;

	if ((dp = opendir (relativePath)) == NULL)
		fprintf(stderr, "Far: cannot opendir(%s)\n", relativePath);

	while ((entry = readdir (dp)))              
	{
		char* name = entry->d_name;
		if(strcmp(".",name) == 0 || strcmp("..",name) == 0)
			continue;
		char* newRelPath = createRel(relativePath,name);
		struct stat statBuf;
		int success = lstat(newRelPath,&statBuf);
		if(success != 0)
			fprintf(stderr,"Far: %s does not exist\n",newRelPath);
		if(success == 0 && S_ISDIR(statBuf.st_mode))
			rkeyHelper(newRelPath,in,fileList);
		else if(success == 0 && S_ISREG(statBuf.st_mode))
			append(in,newRelPath, statBuf,fileList);
		else
			free(newRelPath);
	}
	
	free(relativePath);

	closedir(dp);
}

void rkey(int argc, char* argv[], char* infile)
{
	//check for existence, create if non existent
	FILE* in = fopen(infile,"r");
	if(in == NULL)
		in = fopen(infile,"w");
	if(in == NULL)
	{
		fprintf(stderr,"Far: specified archive, %s, cannot be opened\n",infile);
		exit(0);
	}
	fclose(in);

	dkey(argc, argv,infile,1);

	in = fopen(infile,"a");
	if(in == NULL)
	{
		fprintf(stderr, "Far: Archive does not exist\n");
		exit(0);
	}

	int i;
	struct stat statBuf;
	Flex fileList = newFlex();

	for(i = 3; i < argc; i++)
	{
		//strip trailing '/' from argv[i], if any
		char* name;
		if(argv[i][strlen(argv[i]) - 1] == '/')
		{
			name = malloc(strlen(argv[i])*sizeof(char));
			strncpy(name,argv[i],strlen(argv[i]) - 1);
			name[strlen(argv[i]) - 1] = '\0';
		} else
		{
			name = malloc((strlen(argv[i])+1)*sizeof(char));
			strcpy(name,argv[i]);
		}

		//create the relative path
		char* relativePath = createRel(NULL,name);

		//read file/dir information into statBuf
		int success = lstat(relativePath,&statBuf);
		if(success != 0)
			fprintf(stderr,"Far: %s does not exist\n",relativePath);
		
		//if argument is a directory, start recursion, else proceed normally
		if(success == 0 && S_ISDIR(statBuf.st_mode))
			rkeyHelper(relativePath,in,fileList);
		else if(success == 0 && S_ISREG(statBuf.st_mode))
			append(in,relativePath,statBuf,fileList);
		else
			free(relativePath);

		free(name);
	}

	freeFlex(fileList);
	fclose(in);
}

void tkey(char* infile)
{
	FILE* in = fopen(infile,"r");
	if(in == NULL)
	{
		fprintf(stderr, "Far: Archive does not exist\n");
		exit(0);
	}
	
//	end now represents EOF	
	fseek(in,0,SEEK_END);
	int end = ftell(in);
	rewind(in);

	while(1)
	{
		char fileName[PATH_MAX];
		int byteSize;

//		scan in stuff
		fgets(fileName,PATH_MAX,in);
		fscanf(in,"%d",&byteSize);

//		skip over contents of file
		fseek(in,byteSize+1,SEEK_CUR);
		printf("%8d %s",byteSize,fileName);

//		check for EOF		
		if(ftell(in) >= end) break;
	}

	fclose(in);
}

int main(int argc, char* argv[])
{
	if(argc < 3 || strlen(argv[1]) > 1 || !isalpha(argv[1][0]))
	{
		fprintf(stderr, "Far: Far r|x|d|t archive [filename]*\n");
		exit(0);
	}
	char command = argv[1][0];
	if(!((command == 'r') | (command == 'x') | (command == 't') | (command == 'd')))
	{
		fprintf(stderr, "Far: Far r|x|d|t archive [filename]*\n");
		exit(0);
	}

	char* infile = argv[2];

	switch(command)
	{
		case 'r':
			rkey(argc, argv, infile);
			break;
		case 'x':
			xkey(infile,argv,argc);
			break;
		case 'd':
			dkey(argc, argv, infile,0);
			break;
		case 't':
			tkey(infile);
			break;
		default:
			fprintf(stderr, "Far: you shouldn't have been able to get here...\n");
			break;
	}
	
	return 0;
}
