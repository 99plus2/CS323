#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

/* Print message to stderr using FORMAT that may reference VALUE, then exit */
#define DIE(format,value)  fprintf (stderr, format, value), exit (EXIT_FAILURE)

int main (int argc, char *argv[])
{
	char *name = ".";
	DIR *dp;
	struct dirent *entry;

	if ((dp = opendir (name)) == NULL)          /* Open the directory      */
		DIE ("cannot opendir(%s)\n", name);
	FILE* temp = tmpfile();
	FILE* archive = fopen("./Archive","wb");

	while ((entry = readdir (dp)))              /* Read & print a filename */
	{
		FILE* curFile;
		char* name = entry->d_name;
		printf("NAME: %s\n", name);
		fprintf (temp, "%s\n", name);
		if(!(strcmp(name,".") == 0 | strcmp(name,"..") == 0))
		{
			curFile = fopen(name,"r");
			fseek(curFile,0,SEEK_END);
			int length = ftell(curFile);
			printf("TYPE: %ld|%s\n",length,name);
			fclose(curFile);
		}
	}

	fseek(temp,0L,SEEK_SET);
	while(!feof(temp))
		fputc(fgetc(temp),archive);

	if (fclose(temp) == EOF) printf("AERAWEF");
	if (fclose(archive) == EOF) printf("AEFREGTG");
	closedir (dp);                              /* Close the directory     */

	return EXIT_SUCCESS;
}
