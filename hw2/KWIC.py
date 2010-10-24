#!/usr/bin/python

import sys
import re

#creates the tuple object with format (middle column, file name, line number, left column)
#given line and index (in case there are duplicate words on the same line
def tupleHelper(line,word,startIndex,lineNumber,file):
	position = startIndex+len(word)
	#check if word is substring of larger word, return 0 if true
	if position <= len(line) - 1:
		nextLetter = str(line[position])
		if nextLetter.isalnum() or nextLetter == "_":
			return 0
	middleCol = line[startIndex:startIndex+36]
	middleCol = list(middleCol)
	temp = middleCol.pop()
	if temp != " ": middleCol.append(temp)
	middleCol = ''.join(middleCol)
	startLeftIndex = 0
	#find where to start the left column
	for i in range(startIndex-1,0,-1):
		if not (str(line[i])) == " ":
			startLeftIndex = i
			break;
	#get up to 28 characters to the left of middle column
	if startIndex == 0:
		leftCol = ""
	else:
		leftCol = line[max([0,startLeftIndex-27]):startLeftIndex+1]
	return (middleCol,file, lineNumber,leftCol.rjust(28))

#creates the alphanumeric wordlist, then sends to tupleHelper to get the necessary tuples from the lines
def getTuples(line,lineNumber,file):
	#split words on non alphanumeric chars, then remove duplicates (dealt with later)
	p = re.compile(r'\W+')
	words = p.split(line)
	words = list(set(words))
	listOfTuples = []
	#for some reason, split() returns '' characters, so I had to creae this to get rid of them
	for item in words:
		if len(item) == 0:
			index = words.index(item)
			words = words[:index] + words[index+1:]
	length = len(line)
	if len(words) > 0:
		for word in words:
			i = line.find(word)
			#find all duplicate words on line as well
			while True:
				tuple = tupleHelper(line,word,i,lineNumber,file)
				if tuple != 0:
					listOfTuples.append(tuple)
				i = line.find(word,i+len(word))
				if i == -1:
					break
	return listOfTuples

def main(argv):
	argv = argv[1:]
	listOfTuples = []
	for arg in argv:
		try:
			input = open(arg,'r')
		except:
			print >> sys.stderr, "KWIC: Error opening file %s." % arg
			continue
		#read file bytes, expand the tabs, then split on '\n', then send to getTuples to get the tuples
		file = input.read()
		expandedFile = file.expandtabs()
		lines = expandedFile.splitlines()
		lineNumber = 1
		for line in lines:
			tempListTuples = getTuples(line,lineNumber,arg)
			for tuple in tempListTuples:
				listOfTuples.append(tuple)
			lineNumber += 1
	#create a lambda function that sorts the lower case version of the lists to compare with, then compare with left to right priority
	listOfTuples.sort(lambda (mf,fn,ln,lf),(mf2,fn2,ln2,lf2): cmp((mf.lower(),fn,ln,lf.lower()),(mf2.lower(),fn2,ln2,lf2.lower())))
	#print the list
	for tuple in listOfTuples:
		print "%s   %s  (%s:%d)" % (str(tuple[3]),(str(tuple[0])).ljust(36),str(tuple[1]),tuple[2])

main(sys.argv)
