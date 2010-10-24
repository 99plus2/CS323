Author: Doug von Kohorn
Class: CS323
Professor: Stanley Eisenstat

ESTIMATED time to complete: 20 hours

Date		Start		Time		Description
----		-----		----		-----------
1028		5:00		2:37		Got the main part of history.c fleshed out, except for hExpand(). No real difficulties -- pretty straightforward. Will begin to implement
						hExpand() and lex() tomorrow.
1028		9:00		1:07		Got hExpand fleshed out a bit, but realized that lex() should function first. Worked on that a bit then called it quits.
1031		12:00		2:58		Fleshed out lex.c. Working very well, and will allow me to focus on getting hExpand to work correctly.
1031		6:00		1:56		Working on hExpand. Worried about the right algorithm for searching, because it's hard to discern between !?!?, !?>!?, !??, special cases
						where there is no search activated. Will think of a way to fix in the mean time. World Series is on, so have to stop.
1101		9:45		1:10		Cleaned up a lot of special cases in the code, including those mentioned earlier. Realized that strcspn returns a RELATIVE index based on
						the given pointer, not always relative to the 0th index of the array (bad assumption on my part). That allowed me to set index to the
						correct index AFTER substitution, which wasn't happening before and was working only by fluke. Have every memory function implemented
						except !?SEARCHSTRING?. Will do that later.
1101		8:00		1:30		Got ^,*,$,:M working fairly well -- need to test it out later. Time for Yankees domination.
1103		6:00		1:42		Worked out a bunch of kinks, including hclear and history messing up at 323 commands. Also I had to appease valgrind by memsetting my
						history mechanisms array to null so that I don't have a conditional jump depending on an uninitialized value. Passing all public tests
						now except for the memory one. Will fix that later.
1103		12:45		0:30		Got all the memory blocks freed. Pass all tests. Will torture code later. Also try to shorten the code, history.c is 207 lines.
1105		3:00		1:30		Completed the stress testings for my Lex. Found a few errors in my logic, which made the testing very worthwhile, obviously. Also,
						Yanks won.
				----
		TOTAL time:    15:00

MAJOR DIFFICULTIES:	First major roadblock for me was eventually realizing that I wasn't incrementing index correctly, because strcspn gives RELATIVE position, not absolute position.
			After getting past that, there were just a bunch of off-by-one index errors that I slowly worked through (with generous use of gdb) and fixed. There are a TON
			of logic if-then statements that I feel uncomfortable with because it's very hard to keep all the special cases I check for in my head. The only thing I can do
			to guarantee that they work is to just torture my code, because it's so vast. I'm looking forward to learning from the front end code we receive and realizing
			better ways to organize my code. My final difficulty was making valgrind not complain. First, it took me a WHILE to figure out why it was complaining about
			jumping on uninitialized values, then I realized that memset(tokens,0,323) was not working because the array is much LARGER than 323 bytes. It is 323*sizeof(token*)
			bytes. That got valgrind to stop complaining about conditional jumps, then it started complaining about unfreed blocks, which was all because I forgot to free
			the token** pointer after freeing all its contents. Then I passed all tests. Next, in the stress testing, I found that I allowed !N command to take an N that was
			more than 323 commands away, as well as one that was greater than the given command number. No good. Then I found !-N took and N that was 0 and greater than 323.
			Not good. Then I realized that if the program ended with an hclear, I had a memory error. Not good. Those were my couple of big errors, now I'm confident my program
			is reasonably robust.
