#!/usr/bin/python

# TODO: test scenario

import sys
import os
import subprocess
import socket
import multiprocessing
import time

processes = {}
threads = []
NUM_OF_THREADS = 300

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueRequest(connection, userId, url, expectedResult, timeoutExpected):
	url = url.strip()
	print 'requesting url:', url
	runner.sendRequest(connection, url, userId)
	headers = runner.readResponse(connection, {'result': expectedResult}, timeoutExpected)
	print 'Result:', headers['result']
	print 'all headers:\n', headers

def getValueFromConfigLine(line):
	return line[line.find('=')+1:].rstrip()

def sendFewRequest(opts):
	connection = runner.createConnection('localhost:'+opts.httpdPort)
	issueRequest(connection, opts.userId, "http://www.innasmiesznastrona.pl", runner.ACCESS_OK, timeoutExpected = None)
	issueRequest(connection, opts.userId, "http://pornhub.com", runner.CATEGORY_BLOCKED_CUSTOM, False)
	issueRequest(connection, opts.userId, "http://www.onet.pl", runner.ACCESS_OK, False)

def main():
	(opts, _) = optionParser.parseCommandLine()

	realWebrootPort = 80
	realWebrootHost = 'service2.brightcloud.com'

	webrootCfgPath = 'scripts/tests/webrootUnavailableTest/webroot.cfg'
	fp = open(webrootCfgPath)
	for i, line in enumerate(fp):
		if 'BcapPort' in line:
			opts.tunnelPort = getValueFromConfigLine(line)
		if 'BcapServer' in line:
			tunnelHost = getValueFromConfigLine(line)
	fp.close()
	
	connectionMax = str(NUM_OF_THREADS * 3 + 100)
	httpdArgs = ["--connectionPerIpLimitAbsolute", connectionMax,
		"--connectionPerIpLimitSlowDown", connectionMax,
	]
	
	opts.webrootConfigFilePath = os.path.join(runner.getRepoDir(), webrootCfgPath)
	runner.startTunnel(opts, processes, tunnelHost, realWebrootPort, realWebrootHost)
	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes, httpdArgs)

	runner.setBackendTime(processes, "2014-03-21 16:12:00")

	# we have to create connection here to be reused
	#print 'connecting to db on localhost:%s' % opts.httpdPort
	connection = runner.createConnection('localhost:'+opts.httpdPort)
	issueRequest(connection, opts.userId, "http://www.takatamstronahehe.pl", runner.ACCESS_OK, False)
	issueRequest(connection, opts.userId, "http://pornhub.com", runner.CATEGORY_BLOCKED_CUSTOM, False)
	issueRequest(connection, opts.userId, "http://www.onet.pl", runner.ACCESS_OK, False)
	
	runner.stopTunnel(processes)
	server_address = (tunnelHost, int(opts.tunnelPort))
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
	sock.bind(server_address)
	sock.listen(1)
	
	for i in range(0, NUM_OF_THREADS):
		newThread = multiprocessing.Process(target = sendFewRequest, args = (opts, ))
		newThread.start()
		threads.append(newThread)
	
	time.sleep(10)
	
if __name__ == '__main__':
	try:
		cleanExit = True
		try:
			main()
		except (SystemExit, KeyboardInterrupt):
			pass # --help in arguments
		except:
			cleanExit = False
			runner.testFailed(processes)
			raise
	finally:
		print "Stopping multiprocessing.threads"
		
		runner.teardown(processes)
		
		for t in threads:
			t.join()
			if cleanExit:
				assert t.exitcode == 0, "thread exitcode should be 0 but is %s" % t.exitcode		
