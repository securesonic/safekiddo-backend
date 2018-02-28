#!/usr/bin/python

# TODO: test scenario

import sys
import time
import os

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueRequest(opts, url, expectedResult):
	print 'connecting to localhost:%s' % opts.httpdPort

	connection = runner.createConnection('localhost:' + opts.httpdPort)

	url = url.strip()
	print 'requesting url:', url
	runner.sendRequest(connection, url, opts.userId)
	headers = runner.readResponse(connection, expectedHeaders = {'result': expectedResult})
	print 'Result:', headers['result']
	print 'all headers:\n', headers


def main():
	(opts, _) = optionParser.parseCommandLine()

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-03-24 08:00:01")
	issueRequest(opts, "http://onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)

	runner.setBackendTime(processes, "2014-03-24 10:00:01")
	issueRequest(opts, "http://pornhub.com", runner.ACCESS_OK)

	runner.setBackendTime(processes, "2014-03-23 08:00:01")
	issueRequest(opts, "http://zabijzajaca.pl", runner.ACCESS_OK)

	runner.setBackendTime(processes, "2014-03-23 20:00:01")
	issueRequest(opts, "http://fasdfasdfasdfaqwe.com.pl", runner.INTERNET_ACCESS_FORBIDDEN)

	print "All tests OK"

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
