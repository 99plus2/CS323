Author: Douglas von Kohorn
Class: CS323
Professor: Stanley Eisenstat

ESTIMATED time to complete: 20 hours

Date		Start		Time		Description
----		-----		----		-----------
1116		2:30		2:10		Got simple procedures running. Got redirections running (including HERE documents). Currently contemplating how to start off with pipes. Doing everything recursively
						so far.
1116		8:00		2:30		Got cd [directory] to work. Thinking about how to do pushd and popd. Tried to implement pipes but realized I have an extremely poor understanding of how to throw
						around signals in my program so it knows whats going on, much less what waiting does. Will try to meet with Rich tomorrow to get my comprehension up to par.
1117		3:30		1:10		Got cd to work with background -- realized that I had been forgetting to exit with the correct codes. Need to exit with correct status now. Pipes work as well. Will
						work on passing codes around next (maybe after getting pushd and popd to work).
1117		6:00		3:00		God pushd and popd to work, also got the global errno to be thrown and exited on failure of open or execvp. Still working on statuses in pipes and normal statuses.
						Got background processes to work. Passing 13/18 tests. Need to figure out how to get whether noclobber has been set, and need to figure out how to set $? global
						variable.
1118		12:00		0:30		Figured out that my implementation of pushd and popd won't work for subcommands (since it's a global variable). Due to the public post about how to get the value
						of noclobber, quickly implemented that instead. Will work further on pushd and popd tomorrow.
1118		6:00		1:50		Realized freeing all storage was not required. Also forking a process copies all memory into the child process. Got && and || to work. Passing 17/18 -- pipes are
						still giving me a problem.
1119		12:00		3:00		Finally figured out why my pipes weren't working. Set up piping to stderr. Worked out a couple bugs. Still need to get status setting correct.
1119		3:50		0:50		Switched everything over to process.c. Fixed another bug with piping to stderr.
1119		7:00		1:20		Reread spec. Asked Rich about signal handling regarding SIGINT, realized the parent shouldn't die while waiting for it's child to die if receiving SIGINT. Fixed.
				----
		TOTAL time:    16:20

MAJOR DIFFICULTIES:	Piping was hard only because I forked a process off twice, thereby wasn't receiving the correct exit status. Once this double-forking was realized, the pipe function in my code looked
			much similar to that in pipe.c, and I fixed the signals problem when I realized that I wanted the value of WIFEXITED(status) ? WEXITSTATUS(status) : 128+WTERMSIG(status) instead of just
			status. The next minor problems were finding out how to get the value of noclobber and ? (getenv) and setting them. Valgrind complained at my setting of $? until I memset the variable I
			used to all NULL. The next problem was I had no idea how to handle signals, and the parent process needs to ignore SIGINT while waiting for it's children. After meeting with Rich, I read
			about the signal() function online and got my parent to ignore SIGINT while the children didn't, as specified. Getting signals correct was the hardest part of this assignment -- everything
			seems to work perfectly well once recursively implemented between child and parent.

FIXES:			Fixed the append to work with noclobber as specified.
