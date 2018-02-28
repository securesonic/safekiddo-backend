#!/usr/bin/python
import os
import time
import errno
import fcntl

def isProcessRunning(proc):
	proc.poll()
	return proc.returncode is None

def checkProcessRunning(proc):
	if not isProcessRunning(proc):
		raise Exception("process %s terminated unexpectedly with code %s" % (proc.pid, proc.returncode))

def checkProcessNotRunning(proc):
	if isProcessRunning(proc):
		raise Exception("process %s should be terminated" % proc.pid)

def issueMgmtCommand(pid, name, binDir, cmd):
	if pid is not None:
		checkProcessRunning(pid)
		
	fifoPath = os.path.join(binDir, name + 'Mgmt.fifo')
	print "[%s] issuing management command '%s' to process %s" % (time.ctime(), cmd, name)

	try:
		fd = os.open(fifoPath, os.O_WRONLY | os.O_NONBLOCK)
	except OSError, e:
		if e.errno == errno.ENXIO:
			# process is dead
			print "could not issue management command '%s'" % (cmd,)
			return
		raise

	# O_NONBLOCK was needed only for open
	flags = fcntl.fcntl(fd, fcntl.F_GETFL)
	fcntl.fcntl(fd, fcntl.F_SETFL, flags & ~os.O_NONBLOCK)

	f = os.fdopen(fd, "w")
	print >> f, cmd
	f.close()

	time.sleep(0.1) # UGH! what a nasty hack to wait for command processing