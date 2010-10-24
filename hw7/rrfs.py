#!/c/cs323/Hwk7/systap/python.sh

import sys,re,Systap,os.path,urllib2
matchLocal = re.compile(r'^local:')
matchHost = re.compile(r'^http://')
curSyscallResult = None
globalFileTable = {}
curFileDesc = pow(2,30)

def checkLocal(pid,path,flags,type):
	Systap.setSyscallNum(pid, Systap.ST_SYS_GETPID)
	error = flag = None
	if type == Systap.ST_SYS_OPEN:
		if flags & Systap.ST_O_RDWR:
			error = Systap.ST_EACCES
		elif flags & Systap.ST_O_EXCL and os.path.exists(path):
			error = Systap.ST_EEXIST
		elif not flags & Systap.ST_O_CREAT and not os.path.exists(path):
			error = Systap.ST_ENOENT
	if error == None:
		if flags & Systap.ST_O_APPEND:
			if flags & Systap.ST_O_TRUNC: flag = "w"
			else: flag = "a"
		elif flags & Systap.ST_O_WRONLY and (flags & Systap.ST_O_TRUNC or flags & Systap.ST_O_CREAT): flag = "w"
		elif flags & Systap.ST_O_WRONLY: flag = "w"
		else: flag = "r"
	return (error,flag)

def checkRemote(pid,flags,type):
	Systap.setSyscallNum(pid, Systap.ST_SYS_GETPID)
	error = None
	if type == Systap.ST_SYS_OPEN:
		if flags & Systap.ST_O_WRONLY or flags & Systap.ST_O_RDWR or flags & Systap.ST_O_TRUNC or flags & Systap.ST_O_APPEND or flags & Systap.ST_O_CREAT or flags & Systap.ST_O_EXCL:
			error = Systap.ST_EROFS
	return error

def majority(list):
	ASCII = [0]*256
	for char in list:
		if char != None: ASCII[ord(char)] += 1
	toReturn, max = None, 0
	for i in range(256):
		if ASCII[i] > max: toReturn, max = chr(i), ASCII[i]
	return toReturn

def catchExecution(copies):
	global curFileDesc, curSyscallResult 
	while True:
		rc, pid, event, num, arg0, arg1, arg2, arg3, arg4, arg5, rslt = Systap.getNextTraceEvent()
		if rc >= 0: return rc 

		if event == Systap.ST_EVENT_SYSCALL_ENTERED:
			if num == Systap.ST_SYS_OPEN:
				pathMemAddr, flags = arg0, arg1
				path = Systap.readProcessMemNullTerm(pid, pathMemAddr, 4096)
				type = 0
				flag = None
			
				if matchLocal.search(path):
					curSyscallResult,flag = checkLocal(pid,path[6:],flags,Systap.ST_SYS_OPEN)
					type = 1
				elif matchHost.search(path):
					curSyscallResult = checkRemote(pid,flags,Systap.ST_SYS_OPEN)
					type = 2
				else: curSyscallResult = None
				if curSyscallResult == None and type:
					if type == 1:
						try: file = open(path[6:],flag)
						except:
							curSyscallResult = Systap.ST_EACCES
							continue
					elif type == 2:
						try:
							file = urllib2.urlopen(urllib2.Request(path))
						except:
							curSyscallResult = Systap.ST_ENOENT
							continue
					curSyscallResult = curFileDesc
					globalFileTable[curFileDesc] = file
					curFileDesc += 1
			elif num == Systap.ST_SYS_READ:
				fd, buf, size = arg0, arg1, arg2
				if fd in globalFileTable:
					try:
						output = globalFileTable[fd].read(size*copies)
					except:
						curSyscallResult = Systap.ST_EBADF
						continue
					listOutput, newOutput = list(output), []
					for i in range(0,len(listOutput),copies):
						tempList = []
						for j in range(copies):
							if i+j < len(listOutput): tempList.append(listOutput[i+j])
							else: tempList.append(None)
						newOutput.append(majority(tempList))
					Systap.writeProcessMem(pid,buf,''.join(newOutput))
					curSyscallResult = len(output) / copies
			elif num == Systap.ST_SYS_WRITE:
				fd, buf, size = arg0, arg1, arg2
				if fd in globalFileTable:
					output = list(Systap.readProcessMem(pid,buf,size))
					curSyscallResult = len(output) * copies
					try:
						for char in output:
							for i in range(copies):
								globalFileTable[fd].write(char)
					except:
						curSyscallResult = Systap.ST_EBADF
						continue
			elif num == Systap.ST_SYS_CLOSE:
				fd = arg0
				if fd in globalFileTable:
					try: globalFileTable[fd].close()
					except:
						curSyscallResult = Systap.EBADF
						continue
					curSyscallResult = 0
			else: curSyscallResult = None
		elif event == Systap.ST_EVENT_SYSCALL_EXITED:
			if curSyscallResult != None:
				Systap.setSyscallResult(pid, curSyscallResult)
				curSyscallResult = None

def main():
	programIndex = numCopies = 0
	if sys.argv[1].find("-") == -1: programIndex = 1
	else: numCopies, programIndex = sys.argv[2], 3

	if int(numCopies) != 0 and int(numCopies) % 2 == 0:
		sys.stderr.write("COPIES needs to be odd\n")
		sys.exit(1)
	else: Systap.spawnTraced(sys.argv[programIndex],sys.argv[programIndex:])

	if int(numCopies) == 0: sys.exit(catchExecution(1))
	else: sys.exit(catchExecution(int(numCopies)))

main()
