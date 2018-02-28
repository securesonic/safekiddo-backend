#!/usr/bin/python

import sys
import os
import errno
import signal
import subprocess
import time
import httplib
import fcntl
import copy
import urllib
import mgmtLib

import optionParser

BACKEND = 'backend'
ICAP_SERVER = "c-icap"
HTTPD = 'httpd'
MDM_SERVER = "mdmServer"
REPORTS_SERVER = 'reportsServer'
WEBROOT_OVERRIDES_SCRIPT = "webrootOverridesScript"

BACKEND_EXE = 'backend.exe'
HTTPD_EXE = 'httpd.exe'

DEFAULT_CONFIG = 'debug_64'

def getRepoDir():
	return os.path.abspath(__file__ + '/../../..')

def getBinDir():
	config = os.getenv('CONFIG', DEFAULT_CONFIG)
	return os.path.join(getRepoDir(), 'bin', config)

def makePsqlCmd(opts):
	params = opts
	if not isinstance(params, dict):
		params = params.__dict__

	cmd = 'psql --set ON_ERROR_STOP=1 -h "%(dbHost)s" -d %(dbName)s' % params
	if params['dbUser']:
		cmd += ' -U "%(dbUser)s"' % params
	if 'dbPort' in params:
		cmd += ' -p %(dbPort)s' % params
	return cmd

def loadSqlScript(opts, path):
	print "loading sql script", path
	cmd = makePsqlCmd(opts)
	cmd += ' -f %s' % (path,)
	retcode = subprocess.call(cmd, shell = True, cwd = getRepoDir())
	assert retcode == 0, "could not execute command, retcode: %s" % (retcode,)

def issueSqlCommand(opts, sqlCmd):
	print "issuing sql command:", sqlCmd
	cmd = makePsqlCmd(opts)
	cmd += ' -c "%s"' % (sqlCmd,)
	retcode = subprocess.call(cmd, shell = True)
	assert retcode == 0, "could not execute command, retcode: %s" % (retcode,)

def issuePsqlCommands(opts, sqlCmds):
	print "issuing psql commands"
	cmd = makePsqlCmd(opts)
	proc = subprocess.Popen(cmd, stdin = subprocess.PIPE, shell = True, cwd = getRepoDir())
	proc.communicate(input = sqlCmds)
	assert proc.returncode == 0, "could not execute command, retcode: %s" % (proc.returncode,)

def executeCommand(cmd, cwd = None, stdin = None, stdout = None, stderr = None, disableCoredump = False):
	print "[%s] running: %s" % (time.ctime(), cmd)
	preExecCmd = ''
	if disableCoredump:
		preExecCmd = 'ulimit -c 0; '
	return subprocess.Popen(preExecCmd + 'exec ' + cmd, shell = True, cwd = cwd, stdin = stdin, stdout = stdout, stderr = stderr)

def sendCommandToWebrootOverridesScript(processes, input):
	proc = processes[WEBROOT_OVERRIDES_SCRIPT]
	return proc.communicate(input = input)

def startBackend(opts, processes, addArgs = None):
	address = opts.backendTransport + '*:' + opts.backendPort

	params = copy.copy(opts.__dict__) # what a hack
	params['backendExe'] = BACKEND_EXE
	params['address'] = address

	print "starting backend"
	# some parameters may be empty
	# TODO: dbPassword is empty, in production .pgpass should be used
	cmd = './%(backendExe)s --workers %(workers)d --address "%(address)s" --dbHost "%(dbHost)s" --dbPort %(dbPort)s \
--dbName %(dbName)s --dbNameLogs %(dbNameLogs)s --dbUser "%(dbUser)s" --dbPassword "%(dbPassword)s" \
--logLevel %(logLevel)s --statisticsPrintingInterval %(statisticsPrintingInterval)s --statisticsFilePath "%(statisticsFilePath)s"' \
	% params

	for name in ['dbHostLogs', 'dbPortLogs', 'webrootOem', 'webrootDevice', 'webrootUid', 'webrootConfigFilePath']:
		if params[name] is not None:
			cmd += ' --%s "%s"' % (name, params[name])

	if addArgs is not None:
		for param in addArgs:
			cmd += ' ' + param

	proc = executeCommand(cmd, cwd = getBinDir())
	print "backend started; pid =", proc.pid
	processes[BACKEND] = proc
	# FIXME: we should have some notification mechanism
	# Currently tests start from clean environment and there is webroot db download aborting on clean
	# shutdown, so usually only small webroot db is loaded which normally takes less than 1s.
	time.sleep(3) # UGH! what a nasty hack to wait for backend to load local webroot database
	mgmtLib.checkProcessRunning(proc)

	
def startHttpd(opts, processes, addArgs = None, withFail = False, createAdmissionControlFileIfNotExists = True):
	address = opts.backendTransport + 'localhost:' + opts.backendPort
	
	params = copy.copy(opts.__dict__) # what a hack
	params['httpdExe'] = HTTPD_EXE
	params['backend'] = address

	admissionControlCustomSettingsFile = params['admissionControlCustomSettingsFile']
	# httpd is started with different cwd
	params['admissionControlCustomSettingsFile'] = os.path.abspath(admissionControlCustomSettingsFile)
	if createAdmissionControlFileIfNotExists and not os.path.exists(admissionControlCustomSettingsFile):
		f = open(admissionControlCustomSettingsFile, 'w')
		f.close()

	print "starting httpd"
	cmd = './%(httpdExe)s --port %(httpdPort)s --backend "%(backend)s" --timeout %(timeout)s \
--logLevel %(logLevel)s --admissionControlCustomSettingsFile "%(admissionControlCustomSettingsFile)s"' % params
	if not params['useForwardedFor']:
		cmd += ' --disableForwardedFor'
	if addArgs is not None:
		for param in addArgs:
			cmd += ' ' + param

	proc = executeCommand(cmd, cwd = getBinDir(), disableCoredump = withFail)
	print "httpd started; pid =", proc.pid
	processes[HTTPD] = proc
	time.sleep(1) # UGH! what a nasty hack to wait for httpd to start listening
	if not withFail:
		mgmtLib.checkProcessRunning(proc)
	else:
		mgmtLib.checkProcessNotRunning(proc)

def startMdmServer(processes):
	proc = executeCommand("./mdmServer.py", cwd = os.path.join(getRepoDir(), 'mdm/wp8'))
	print "mdmServer started; pid =", proc.pid
	processes[MDM_SERVER] = proc
	#Wait for cherrypy start
	time.sleep(2)
	mgmtLib.checkProcessRunning(proc)
	
def startReportsServer(processes):
	proc = executeCommand("./reportsServer.py", cwd = os.path.join(getRepoDir(), 'web/reports'))
	print "reportsServer started; pid =", proc.pid
	processes[REPORTS_SERVER] = proc
	#Wait for cherrypy start
	time.sleep(2)
	mgmtLib.checkProcessRunning(proc)

def startIcapServer(processes):
	try:
		os.mkdir(os.path.join(getRepoDir(), 'c-icap/logs'))
	except OSError:
		pass

	proc = executeCommand("c-icap -f configs/c-icap.conf -N -D", cwd = os.path.join(getRepoDir(), 'c-icap'))
	print "icap server started; pid =", proc.pid
	processes[ICAP_SERVER] = proc
	time.sleep(2)
	mgmtLib.checkProcessRunning(proc)

def runIcapClient(url):
	proc = executeCommand("c-icap-client -s safekiddo -p 11344 -req '" + url + "' -d 3 -v", stderr = subprocess.PIPE)
	print "icap client started; pid =", proc.pid
	response = proc.stderr.read()
	retcode = proc.wait()
	assert retcode == 0, "icap client exited with unexpected code %d" % retcode
	return response
		
def startWebrootOverridesScript(processes):
	proc = executeCommand(
		"./webrootOverridesScript.py", 
		os.path.join(getRepoDir(), 'scripts/webrootOverridesScript'), 
		subprocess.PIPE, 
		subprocess.PIPE
	)
	print "webrootOverrideScript started; pid =", proc.pid
	processes[WEBROOT_OVERRIDES_SCRIPT] = proc
	#Wait for cherrypy start
	time.sleep(2)
	mgmtLib.checkProcessRunning(proc)
	
def startTunnel(opts, processes, host, port, remote = 'localhost'):
	if host == "":
		host = 'localhost'
	cmd = 'ssh -L %s:%s:%s %s -N' % (opts.tunnelPort, remote, port, host)
	proc = executeCommand(cmd)
	print "tunnel started; pid =", proc.pid
	processes['tunnel'] = proc
	time.sleep(1) # UGH! what a nasty hack to wait for tunnel to be established
	mgmtLib.checkProcessRunning(proc)

def killProcess(name, processes, signalnum = signal.SIGTERM):
	proc = processes[name]
	if mgmtLib.isProcessRunning(proc):
		if name == BACKEND and signalnum == signal.SIGTERM:
			print '[%s] shutting down backend (cleanly)' % (time.ctime(),)
			mgmtLib.issueMgmtCommand(processes[name], name, getBinDir(), 'stop')
			del processes[name]
			retcode = proc.wait()
			assert retcode == 0, "backend exited with unexpected code %d" % retcode
		else:
			# Flush logs only on test failure (on which processes are killed with SIGILL).
			# There is no clean shutdown of httpd, but its logs are usually not important in case of
			# test success. Moreover logs are written to a file with no delay usually.
			if signalnum == signal.SIGILL and (name == BACKEND or name == HTTPD):
				mgmtLib.issueMgmtCommand(processes[name], name, getBinDir(), 'flushLogs')
				# logs flushing on icap-dev sometimes took a few seconds
				time.sleep(5)
			del processes[name]
			print '[%s] killing process %s pid %d with signal %s' % (time.ctime(), name, proc.pid, signalnum)
			# first resume the process if it was suspended
			subprocess.call('kill -%d %d' % (signal.SIGCONT, proc.pid), shell = True)
			subprocess.call('kill -%d %d' % (signalnum, proc.pid), shell = True)
			if signalnum != signal.SIGILL: # in case of coredump we don't care
				time.sleep(0.1) # time for the process to finish
				if mgmtLib.isProcessRunning(proc):
					print 'killProcess: process still exists, waiting longer', name, proc.pid
					time.sleep(1)
					if mgmtLib.isProcessRunning(proc):
						print 'killProcess: timed out waiting for process to finish, forcing a coredump', name, proc.pid
						subprocess.call('kill -%d %d' % (signal.SIGILL, proc.pid), shell = True)
						proc.wait()
	else:
		del processes[name]
		if proc.returncode == 0:
			print 'killProcess: process does not exist', name, proc.pid
		else:
			print '[%s] Error: process terminated with code %d, please see the logs!' % (time.ctime(), proc.returncode), name, proc.pid

def setBackendTime(processes, currentTime):
	mgmtLib.issueMgmtCommand(processes[BACKEND], BACKEND, getBinDir(), 'setCurrentTime ' + str(currentTime))

def reloadBackendOverrides(processes):
	mgmtLib.issueMgmtCommand(processes[BACKEND], BACKEND, getBinDir(), 'reloadOverrides')

def reloadHttpdAdmissionControlCustomSettingsFile(processes):
	mgmtLib.issueMgmtCommand(processes[HTTPD], HTTPD, getBinDir(), 'reloadAdmissionControlCustomSettingsFile')
	time.sleep(2)

def suspendProcess(name, processes):
	proc = processes[name]
	print '[%s] suspending process' % (time.ctime(),), name, proc.pid
	subprocess.call('kill -STOP %d' % proc.pid, shell = True)

def stopBackend(processes):
	killProcess(BACKEND, processes)

def stopHttpd(processes):
	killProcess(HTTPD, processes)
	
def stopWebrootOverridesScript(processes):
	killProcess(WEBROOT_OVERRIDES_SCRIPT, processes)

def stopTunnel(processes):
	killProcess('tunnel', processes)

def setup(opts):
	processes = {}
	startBackend(opts, processes)
	startHttpd(opts, processes)
	return processes

def teardown(processes):
	printEnvDebugInfo(processes, "env_info_beforeTeardown.txt")
	for name in processes.keys():
		killProcess(name, processes)

def killAllInstances():
	subprocess.call('killall -15 %s' % BACKEND_EXE, shell = True)
	subprocess.call('killall -15 %s' % HTTPD_EXE, shell = True)

def testFailed(processes):
	printEnvDebugInfo(processes, "env_info_beforeForcedKill.txt")
	print '[%s] Forcing processes to dump core' % (time.ctime(),)
	for name in processes.keys():
		killProcess(name, processes, signalnum = signal.SIGILL)

def printEnvDebugInfo(processes, fileName):
	f = open(fileName, "a")
	print >> f, '[%s] Printing environment debug information' % (time.ctime(),)
	f.close()
	subprocess.call('sudo netstat -anp >> %s' % fileName, shell = True)
	subprocess.call('ps wfaxo stat,euid,ruid,tty,pgrp,ppid,pid,pcpu,pmem,time,args >> %s' % fileName, shell = True)

if __name__ == '__main__':
	(opts, args) = optionParser.parseCommandLine()

	if opts.clear:
		killAllInstances()
	
	processes = setup(opts)
	print 'press enter to tear down'
	sys.stdin.readline()
	teardown(processes)

################## TODO: move below methods to a better place #####################

def createConnection(host, isHttps = False):
	if isHttps:
		conn = httplib.HTTPSConnection(host)
	else:
		conn = httplib.HTTPConnection(host)
	
	conn.connect()
	fd = conn.sock.fileno()
	# backend must have enough file descriptors available
	flags = fcntl.fcntl(fd, fcntl.F_GETFD)
	fcntl.fcntl(fd, fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)
	return conn

def quoteUrl(url):
	start = url.find('://')
	if start == -1:
		start = 0
	else:
		start = start + 3
	pathPos = url.find('/', start)
	if pathPos == -1:
		pathPos = len(url)
	return url[:pathPos] + urllib.quote(url[pathPos:])

def sendRequest(connection, url, userId, quote = True, userAction = None, addHeaders = {}):
	if quote:
		url = quoteUrl(url)
	headers = { 'Request' : url, 'UserId' : userId }
	if userAction is not None:
		headers['UserAction'] = str(userAction)
	for header, value in addHeaders.iteritems():
		headers[header] = value
	connection.request('GET', '/cc', headers = headers)

def isTimeout(headers):
	return 'x-result' in headers and 'timeout' in headers['x-result']

# return a dictionary of all headers
def readResponse(connection, expectedHeaders = {}, timeoutExpected = False, expectedHttpStatus = 200):
	response = connection.getresponse()
	response.read()
	assert response.status == expectedHttpStatus, "unexpected http status: %s, expected %d" % (response.status, expectedHttpStatus)
	if expectedHttpStatus != 200:
		return
	headers = dict(response.getheaders())

	if timeoutExpected is not None:
		if timeoutExpected and not isTimeout(headers):
			assert False, "timeout was expected, headers: %s" % (headers,)
		elif not timeoutExpected and isTimeout(headers):
			# Backend may be dumping core at the moment if it detected e.g. slow logging device.
			# Backend may got clogged for a moment, if we give it more time it may produce some helpful logs.
			print "[%s] WS returned timeout, sleeping in case backend dumps core at the moment" % (time.ctime(),)
			time.sleep(20) # 10s might not be enough
			assert False, "timeout was not expected, headers: %s" % (headers,)
	for key, value in expectedHeaders.iteritems():
		assert headers[key] == value, "header '%s' is %s but expected %s" % (key, headers[key], value)
	return headers

ACCESS_OK = '0'
INTERNET_ACCESS_FORBIDDEN = '100'
CATEGORY_BLOCKED_CUSTOM = '111'
CATEGORY_BLOCKED_GLOBAL = '112'
URL_BLOCKED_CUSTOM = '121'
URL_BLOCKED_GLOBAL = '122'
IP_REPUTATION_CHECK_FAILED = '200'
UNKNOWN_USER = '300'

class UserActionCode:
	NON_INTENTIONAL_REQUEST = 0
	INTENTIONAL_REQUEST = 1
	CATEGORY_GROUP_QUERY = 2
