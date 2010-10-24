#!/usr/bin/python
# Author: Douglas von Kohorn
# Professor: Stanley Eisenstat
# Class: CS323
import sys,re,pdb

#I got tired of passing all these regex around, so I made them global
matchElse, matchEnd = re.compile(r'^\s*#else\s*$'), re.compile(r'^\s*#endif\s*$')
matchDef = re.compile(r'^\s*(#define\b)|^\s*(#undef\b)')
matchIfDef = re.compile(r'^\s*#ifdef\s*(\w+)\s*$')
matchElseEnd = re.compile(r'^\s*#else\s*$|^\s*#endif\s*$')
macros = {}

#expand a given line
def expand(line):
	#finds the next macro in the line that is being expanded
	findMacro = re.compile(r'[^#^\n]*(#{(\w+)})|(#(\w+))')
	#if there are no more to find, return the line
	try: groups = sorted(list(set(findMacro.search(line).groups())))
	except: return line
	#dirtyExpander is needed to find the index of macro in line
	dirtyExpander, cleanExpander = groups[1], groups[2]
	index = line.find(dirtyExpander)
	#repeat logic from above until there are no more macros to be expanded
	while(cleanExpander != None):
		try: line = line[0:index] + line[index:].replace(dirtyExpander,macros[cleanExpander],1)
		except: index += len(dirtyExpander)
		try:
			groups = sorted(list(set(findMacro.search(line[index:]).groups())))
			dirtyExpander,cleanExpander = groups[1],groups[2]
			index = line.find(dirtyExpander,index)
		except: cleanExpander = None
	return line

#update the dictionary given a call (word) and a line
def updateDict(line,word):
	#this regex extracts the correct components from the word call
	keyVal = list(re.compile(r'%s\s*(\w+)\s*(.*\S)?\s*' % word[1]).search(line).groups())
	#if second value is not found, set to empty string, then update dict
	#according to the call (add or delete)
	if keyVal[1] == None:
		keyVal[1] = ""
	if(word[1][1:] == "define"):
		macros[keyVal[0]] = keyVal[1]
	else:
		try: del macros[keyVal[0]]
		except: pass

#statements to check for correctness of ifdef statements
#and if correct, operate on them just like main().
#shouldExpand is true if the current block of code should be
#operated on
def recurseIfDef(macro,lineNumber,lines,shouldExpand):
	#matching variables, inBlock is True if the current frame should
	#be treated as normal, False if should be ignored and just checked
	#for correct syntax
	elseMatched,endMatched = False,False
	inBlock = ((macro in macros) and shouldExpand)
	while lineNumber < len(lines):
		line = lines[lineNumber]
		#if else is true when set, then too many else's, otherwise set inBlock to correct value
		if matchElse.search(line):
			if elseMatched == False: elseMatched = True; lineNumber += 1; inBlock = (shouldExpand and not inBlock)
			else: print >> sys.stderr, "M09: missing #endif."; sys.exit(0)
		#if else not matched yet, throw error, otherwise break from syntax logic
		elif matchEnd.search(line):
			if elseMatched == True: endMatched = True; lineNumber += 1; break
			else: print >> sys.stderr, "M09: missing #else."; sys.exit(0)
		#recurse deeper
		elif matchIfDef.search(line):
			lineNumber = recurseIfDef(matchIfDef.search(line).group(1),lineNumber+1,lines,inBlock)
		#update dictinory only if this frame matters
		elif matchDef.search(line) and shouldExpand and inBlock:
			updateDict(line,sorted(matchDef.search(line).groups()))
			lineNumber += 1
		#expand only if this frame matters, otherwise ignore and increment
		elif shouldExpand and inBlock: print expand(line); lineNumber += 1
		else: lineNumber += 1
	if elseMatched and endMatched: return lineNumber
	else: print >> sys.stderr, "M09: missing closing statements to #ifdef."; sys.exit(0)

#goes through all the lines (except for the ones contained w/in if-else-end)
def main():
	lines = sys.stdin.read().splitlines()
	lineNumber = 0
	#go through lines, find out what type of action should be taken
	#recurse if #ifdef, update macros if #def/#undef, otherwise expand
	while lineNumber < len(lines):
		line = lines[lineNumber]
		if matchDef.search(line):
			updateDict(line,sorted(matchDef.search(line).groups()))
			lineNumber += 1
		elif matchElseEnd.search(line):	print >> sys.stderr, "M09: missing #ifdef."; sys.exit(0)
		elif matchIfDef.search(line):
			lineNumber = recurseIfDef(matchIfDef.search(line).group(1),lineNumber+1,lines,True)
		else: print expand(line); lineNumber += 1
main()
