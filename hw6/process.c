/*
 * Author: Douglas von Kohorn
 * Professor: Stanley Eisenstat
 * Class: CS323
 */
#define errorExit(prog, status)  perror(prog), exit(status)

#include "/c/cs323/Hwk6/process.h"
#include "stack.h"

//global variables for noclobber, stack memory, and file permissions
static char* NOCLOBBER;
static Stack DIRSTACK;
static mode_t PERMISSION = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

//function prototype
int createChild(CMD* cmd);

//returns 1 if argv[0] is a builtin, 0 otherwise
int builtIn(CMD* cmd)
{
	if((cmd->argc > 0))
	{
		char* arg = cmd->argv[0];
		return (!strcmp(arg,"cd")) ||
			(!strcmp(arg,"pushd")) || (!strcmp(arg,"popd"));
	}
	return 0;
}

//sets the status of $? to last executed command
void setStatus(int num)
{
	char env[10];
	memset(env,0,10*sizeof(char));
	sprintf(env,"%d",num);
	setenv("?",env,1);
}

//redirects input and output to the values specified by cmd
void setRedirects(CMD* cmd)
{
	//here, herePID, and temp are the for heredocs
	int fdin, fdout, status, here[2], herePID;
	FILE* temp;
	
	//simple input file
	if(cmd->fromType == RED_IN)
	{
		if((fdin = open(cmd->fromFile, O_RDONLY)) == -1)
			errorExit(cmd->fromFile,errno);
		dup2(fdin,0);
		close(fdin);
	//heredoc input
	} else if(cmd->fromType == RED_HERE)
	{
		//create pipe
		pipe(here);
		if((herePID = fork()) < 0)
			errorExit(cmd->argv[0],errno);	
		else if(herePID == 0)
		{
			//input from pipe comes from file in parent
			close(here[1]);
			dup2(here[0],0);
			close(here[0]);
		} else
		{
			//output file to pipe
			close(here[0]);
			if((temp = fdopen(here[1],"w")) == NULL)
				errorExit(cmd->argv[0],errno);
			fputs(cmd->fromFile,temp);
			fclose(temp);
			wait(&status);
			exit(status);
		}
	}

	//append to file, make sure not to clobber if clobber is set
	if(APPEND(cmd->toType))
	{
		if(CLOBBER(cmd->toType) || NOCLOBBER == NULL)
		{
			if((fdout = open(cmd->toFile, 
			O_WRONLY|O_CREAT|O_APPEND,PERMISSION)) == -1)
				errorExit(cmd->toFile,errno);
		} else if (NOCLOBBER != NULL)
		{
			if((fdout = open(cmd->toFile, 
			O_WRONLY|O_APPEND,PERMISSION)) == -1)
				errorExit(cmd->toFile,errno);
		}
		dup2(fdout,1);
		close(fdout);
	//output to file, destroy if clobber or noclobber == NULL, otherwise make sure
	//file doesn't exist
	} else if((ERROR(cmd->toType) || CLOBBER(cmd->toType) 
		|| cmd->toType == RED_OUT) && !PIPE(cmd->toType))
	{
		if(CLOBBER(cmd->toType) || NOCLOBBER == NULL)
		{
			if((fdout = open(cmd->toFile,
			O_WRONLY|O_CREAT|O_TRUNC,PERMISSION)) == -1)
				errorExit(cmd->toFile,errno);
		} else if (NOCLOBBER != NULL)
		{
			if((fdout = open(cmd->toFile,
			O_WRONLY|O_CREAT|O_EXCL,PERMISSION)) == -1)
				errorExit(cmd->toFile,errno);
		}
		dup2(fdout,1);
		close(fdout);
	}

	//if redirect error, then set stderr to where stdout is pointing
	if(ERROR(cmd->toType) && !PIPE(cmd->toType)) dup2(1,2);
}

//executes the builtin command in cmd
int executeBuiltIn(CMD* cmd)
{
	char **argv = cmd->argv, *env;
	int argc = cmd->argc;

	//if cd, try to cd if correct args, otherwise return > 1
	if(!strcmp(argv[0],"cd"))
	{
		if(argc > 2) return 1;
		if(argc == 1)
		{
			if((env = getenv("HOME")) == NULL) return 1;
			if(chdir(env) < 0) return errno;
		} else if(chdir(argv[1]) < 0) return errno;
	//if pushd, add the path only if argv[1] exists, otherwise error and
	} else if(!strcmp(argv[0],"pushd"))
	{
		if(argc != 2) return 1;
		else
		{
			long size;
			char *buf, *cwd;

			//get absolute path size, then add the malloc'd string
			size = pathconf(".",_PC_PATH_MAX);
			buf = malloc(size);
			cwd = getcwd(buf,size);
			if(cwd == NULL) return errno;
			pushStack(DIRSTACK,cwd);

			if(chdir(argv[1]) < 0)
			{
				popStack(DIRSTACK);
				return errno;
			}
		}
	//if popd, make sure proper number of args and stack not empty
	//chdir, return if any error occurs
	} else if(!strcmp(argv[0],"popd"))
	{
		if(isEmptyStack(DIRSTACK) || argc > 1) return 1;
		else
		{
			char* dir = popStack(DIRSTACK);
			if(chdir(dir) < 0) return errno;
		}
	}
	return 0;
}

//wrapper for executeBuiltin, determines whether it should be
//executed in the background or not
int makeBuiltIn(CMD* cmd)
{
	int pid, status;

	//if in background, fork child, execute, but have no parent waiting on it
	if(cmd->sep == SEP_BG)
	{
		if((pid = fork()) < 0) errorExit(cmd->argv[0],errno);
		else if(pid == 0)
		{
			//record status so it can print any errors
			status = executeBuiltIn(cmd);
			if(status) fprintf(stderr,"%s: incorrect usage.\n",cmd->argv[0]);
			exit(status);
		}
	//otherwise return status but fork no child
	} else
	{
		status = executeBuiltIn(cmd);
		if(status) fprintf(stderr, "%s: incorrect usage.\n", cmd->argv[0]);
		return status;
	}
	return 0;
}

//adaptation of pipe.c, returns the exit status of the most
//recently failed command
int processPipe(CMD* cmd)
{
	struct entry
	{
		int pid, status;
	} *table;

	int fd[2],                  // Read and write file descriptors for pipe
	pid,                        // Process ID for child process
	status = 0,                 // Status of child process
	fdin,                       // Read end of last pipe (or original stdin)
	i,j,result = 0,argc=0;      // Remember last failed command
	CMD *original = cmd, *temp; // Keep track of commands
    
    	//find out how many pipes are needed
	for(temp = cmd; temp != NULL; temp = temp->next) argc++;

	//malloc a table large enough to hold their pid and status
	table = malloc (argc * sizeof(*table));

	//create chain of processes
	fdin = 0;                                 
	for (i = 0; i < argc-1; i++)
	{
		pipe (fd);
		if ((pid = fork()) < 0) errorExit (cmd->argv[0],errno);
		else if (pid == 0)
		{
			close (fd[0]);
			if (fdin != 0) 
			{
				dup2 (fdin, 0);
				close (fdin);
			}
			if (fd[1] != 1)
			{
				dup2 (fd[1], 1);
				if(ERROR(original->toType)) dup2(1,2);
				close (fd[1]);
			}

			//execute chain of commands
			status = createChild(original);
			exit(status);
		} else
		{
			table[i].pid = pid;

			if (i > 0) close (fdin);

			fdin = fd[0];
			close (fd[1]);
		}

		//next command in pipe
		original = original->next;
	}

	//create last pipe in chain
	if ((pid = fork()) < 0) errorExit(cmd->argv[0],errno);
	else if (pid == 0)
	{
		if (fdin != 0)
		{
			dup2 (fdin, 0);
			close (fdin);
		}

		//execute last command
		status = createChild(original);
		exit(status);
	} else
	{
		table[argc-1].pid = pid;
		if (i > 0) close (fdin);
	}
	 
	for (i = 0; i < argc; )
	{
		pid = wait (&status);

		//remember their status
		for (j = 0; j < argc && table[j].pid != pid; j++) ;
		if (j < argc)
		{
			table[j].status = status;
			i++;
		}
	}

	//set result to status of earliest pipe that failed, return result
	for(j = 0; j < argc; j++)
	{
		if(WIFEXITED(table[j].status))
		{
			if(WEXITSTATUS(table[j].status) != 0 && result == 0)
				result = WEXITSTATUS(table[j].status);
		} else
		{
			if((128+WTERMSIG(table[j].status)) != 0 && result == 0)
				result = (128+WTERMSIG(table[j].status));
		}
	}
	free(table);
	return result;
}

//executes command, recurses if there is a pipe or subcommand
int createChild(CMD* cmd)
{
	int status = 0;

	setRedirects(cmd);

	//recurse deeper if subCmd or pipe
	if(cmd->subCmd != NULL) status = process(cmd->subCmd);
	if(cmd->pipe != NULL) status = processPipe(cmd->pipe);

	//execute command
	if(cmd->argc > 0 && builtIn(cmd)) status = makeBuiltIn(cmd);
	else if(cmd->argc > 0)
	{
		if((execvp(cmd->argv[0],cmd->argv)) == -1)
		{
			status = errno;
			perror(cmd->argv[0]);
		}
	}

	return status;
}

//builtins take care of their own status otherwise
//waits for child, ignores SIGINT,and returns status
int modWait(int pid, int builtIn)
{
	if(builtIn) return 0;
	else
	{
		//oldVal holds the function returned by signal()
		int status;
		void (*oldVal)();

		//makes sure that wait doesn't return on keyboard interrupt
		oldVal = signal(SIGINT, SIG_IGN);
		waitpid(pid,&status,0);
		signal(SIGINT,oldVal);

		return (WIFEXITED(status) ? 
				WEXITSTATUS(status) : 128+WTERMSIG(status));
	}
}

//handles the logic for jumping to the next command based
//on execution of the previous (recursion). sets $? status
int continueExecution(CMD* cmd, int pid, int builtIn)
{
	int status = 0;
	
	//run in foreground, proceed to next module (if any)
	if(cmd->sep == SEP_END)
	{
		if(!(PIPE(cmd->toType) || PIPE(cmd->fromType)))
		{
			status = modWait(pid,builtIn);
			setStatus(status);
			if(cmd->next) status = process(cmd->next);
		}
	//run in background (don't wait for completion)
	} else if(cmd->sep == SEP_BG)
	{
		if(cmd->next) status = process(cmd->next);
		else return 0;
	//if OR, run only if first command fails
	//if AND, run only if first command works
	} else if((cmd->sep == SEP_OR) || (cmd->sep == SEP_AND))
	{
		if(!(PIPE(cmd->toType) || PIPE(cmd->fromType)))
		{
			status = modWait(pid,builtIn);
			setStatus(status);
			if(status != 0 && cmd->sep == SEP_OR) status = process(cmd->next);
			if(status == 0 && cmd->sep == SEP_AND) status = process(cmd->next);
		}
	}

	return status;
}

int process(CMD* cmd)
{
	int pid, status, backup[3];
	if(DIRSTACK == NULL) DIRSTACK = newStack();
	NOCLOBBER = getenv("noclobber");

	//builtIn works on the parent process -- no fork necessary
	if(builtIn(cmd))
	{
		//remember previous values in case there is redirection
		for(int i = 0; i < 3; i++) backup[i] = dup(i);
		
		status = createChild(cmd);
		setStatus(status);

		//restore prev vals, close dups
		for(int i = 0; i < 3; i++)
		{
			dup2(backup[i],i);
			close(backup[i]);
		}

		//if didn't fail, continue
		if(!status) status = continueExecution(cmd,0,1);
	} else if((pid = fork()) < 0)
	{
		errorExit(cmd->argv[0],errno);
	} else if(pid == 0)
	{
		//execute command (possibly recurse deeper)
		status = createChild(cmd);
		exit(status);
	} else
		//continue to next command (possibly recurse deeper)
		status = continueExecution(cmd,pid,0);
	return status;
}

