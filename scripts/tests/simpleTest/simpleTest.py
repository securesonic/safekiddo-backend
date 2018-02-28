#!/usr/bin/python

# Test scenario:
# 1. start webservice
# 2. send a few requests (serialized), expect reply after a timeout
# 3. start backend
# 4. send a few requests (serialized), expect reply fast[er]

import sys
import time
import os

TOLERANCE = 0.05
NUM_REQUESTS = 5

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def almostEqual(x, y, tolerance):
	med = (x + y) / 2
	dx = abs(x - med)
	dy = abs(y - med)
	allowedDiff = abs(tolerance * med) / 2
	return (dx < allowedDiff) and (dy < allowedDiff)

def issueSequentialRequests(opts, urls, timeoutExpected = False):
	print 'connecting to localhost:%s' % opts.httpdPort
	
	connection = runner.createConnection('localhost:' + opts.httpdPort)

	for url in urls:
		url = url.strip()
		print 'requesting url:', url
		startTime = time.time()
		runner.sendRequest(connection, url, opts.userId)
		headers = runner.readResponse(connection, timeoutExpected = timeoutExpected)
		duration = time.time() - startTime
		print 'Result:', headers['result']
		print 'all headers:\n', headers
		print 'duration: %.2f' % duration

def main():
	(opts, _) = optionParser.parseCommandLine()
	opts.timeout = 1000 # 1sec timeout

	runner.startHttpd(opts, processes)

	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:NUM_REQUESTS]

	startTime = time.time()
	issueSequentialRequests(opts, urls, timeoutExpected = True)
	duration = time.time() - startTime
	expectedDuration = NUM_REQUESTS * float(opts.timeout) / 1000
	assert almostEqual(duration, expectedDuration, TOLERANCE), "expected timed out requests: %f vs %f" % (duration, expectedDuration)

	runner.startBackend(opts, processes)

	startTime = time.time()
	issueSequentialRequests(opts, urls)
	duration = time.time() - startTime

	if opts.dbHost == "" or opts.dbHost == "localhost":
		print "db is local"
		expectedDuration = expectedDuration / 10
	else:
		print "db is remote"
		expectedDuration = expectedDuration / 2
	assert duration < expectedDuration, "requests duration: %f, expected less than %f" % (duration, expectedDuration)

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
