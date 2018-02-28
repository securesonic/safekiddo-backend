#!/usr/bin/python

import sys
import time
import os
import copy

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueRequests(opts, urls):
	print 'connecting to localhost:%s' % opts.httpdPort

	connection = runner.createConnection('localhost:' + opts.httpdPort)
	startTime = time.time();

	for url in urls:
		url = url.strip()
		print 'requesting url:', url
		runner.sendRequest(connection, url, opts.userId)
		runner.readResponse(connection, expectedHeaders={'result': runner.ACCESS_OK})

	return time.time() - startTime

def main():
	(opts, _) = optionParser.parseCommandLine()

	requestLoggerMaxQueueLength = 10000

	if opts.dbHost == "" or opts.dbHost == "localhost":
		print "db is local"
		numUrls = 15000
	else:
		print "db is remote"
		numUrls = 1000

	print "Overriding logLevel to INFO"
	opts.logLevel = 'INFO'

	optsOriginal = copy.copy(opts)
	opts.dbHostLogs = 'localhost'
	opts.dbPortLogs = optsOriginal.tunnelPort

	tunnelHost = optsOriginal.dbHostLogs
	if tunnelHost is None:
		tunnelHost = optsOriginal.dbHost

	tunnelPort = optsOriginal.dbPortLogs
	if tunnelPort is None:
		tunnelPort = optsOriginal.dbPort

	runner.startTunnel(opts, processes, tunnelHost, tunnelPort)

	# The test verifies that backend performance is not degraded in case logs db becomes sluggish.
	# Cloud access is too slow for this test.
	backendArgs = [
		'--requestLoggerMaxQueueLength', str(requestLoggerMaxQueueLength),
		'--setCurrentTime', '"2014-03-21 15:31:00"',
		'--disableWebrootCloud'
	]
	runner.startBackend(opts, processes, backendArgs)

	# Backend should now have loaded webroot local db, but a connection pool to logs db is created
	# after webroot gets initialized.
	time.sleep(3)
	# Suspend connections to logs db.
	runner.suspendProcess('tunnel', processes)

	httpdArgs = [
		"--connectionPerIpLimitAbsolute", str(numUrls),
		"--connectionPerIpLimitSlowDown", str(numUrls),
	]
	runner.startHttpd(opts, processes, httpdArgs)

	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:numUrls]

	duration = issueRequests(opts, urls)
	performance = len(urls) / duration
	print 'reqs/s: %.1f (duration %.2f, urls %d)' % (performance, duration, len(urls))

	assert performance > 400, "Performance is lower than expected"

	output = open("performance.txt", "w")
	output.write(str(performance))
	output.close()

if __name__ == '__main__':
	try:
		try:
			main()
		except (SystemExit, KeyboardInterrupt):
			pass # --help in arguments
		except:
			runner.testFailed(processes)
			raise
	finally:
		print 'Tearing down'
		runner.teardown(processes)
