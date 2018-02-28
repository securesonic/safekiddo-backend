#!/usr/bin/python

# Test scenario:
# 0. set long timeout
# 1. start webservice
# 2. send a feq requests (parallel), wait half of the timeout
# 3. start backend
# 4. expect responses prior to timeout

import sys
import time
import os
import signal

NUM_REQUESTS = 200 # no of first urls from alexa list
processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def startConcurrentRequests(opts, urls):
	print 'connecting to localhost:%s' % opts.httpdPort

	pool = []
	for _ in xrange(len(urls)):
		pool.append(runner.createConnection('localhost:' + opts.httpdPort))

	for idx in xrange(len(urls)):
		url = urls[idx]
		url = url.strip()
		print 'requesting url:', url
		connection = pool[idx]
		runner.sendRequest(connection, url, opts.userId)

	return pool

def checkResponses(pool):
	for connection in pool:
		headers = runner.readResponse(connection)
		print 'Result:', headers['result']
		print 'all headers:\n', headers

def main():
	(opts, args) = optionParser.parseCommandLine()

	if opts.dbHost == "" or opts.dbHost == "localhost":
		print "db is local"
		opts.timeout = 10000
		expectedDuration = 2
	else:
		print "db is remote"
		opts.timeout = 20000
		expectedDuration = 4

	runner.startHttpd(opts, processes)

	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:NUM_REQUESTS]

	pool = startConcurrentRequests(opts, urls)

	sleepTime = opts.timeout / 2 / 1000 # sleep half of timeout
	print 'waiting for %.2fs' % sleepTime
	class Timeout:
		pass
	def handler(signum, frame):
		raise Timeout()
	signal.signal(signal.SIGALRM, handler)
	signal.alarm(sleepTime)
	try:
		checkResponses(pool)
	except Timeout:
		pass
	else:
		assert False, 'requests finished too fast'
	signal.alarm(0)

	# When cloud is used:
	# Requests are not dropped until backend starts and on start it has to process them all when db is
	# not yet loaded, so they are sent to the cloud. Loading of full db takes more than 5s.
	# Therefore we disable cloud.
	backendArgs = ['--disableWebrootCloud', '--disableWebrootLocalDb']
	runner.startBackend(opts, processes, backendArgs)

	startTime = time.time()

	checkResponses(pool)

	duration = time.time() - startTime
	print 'handling duration: %.2fs' % duration
	assert duration < expectedDuration, "all requests should have finished within %.2fs after backend start; instead got: %.2fs" % \
		(expectedDuration, duration)

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
