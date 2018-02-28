#!/usr/bin/python

# TODO: test scenario

import sys
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

	runner.setBackendTime(processes, "2014-03-21 15:31:00")
	issueRequest(opts, "http://onet.pl", runner.ACCESS_OK)

	runner.setBackendTime(processes, "2014-03-21 18:12:00")
	issueRequest(opts, "http://www.onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)

	runner.setBackendTime(processes, "2014-03-21 08:03:00")
	issueRequest(opts, "http://pornhub.com", runner.CATEGORY_BLOCKED_CUSTOM)

	runner.setBackendTime(processes, "2014-03-21 07:59:00")
	issueRequest(opts, "http://pornhub.com", runner.INTERNET_ACCESS_FORBIDDEN)

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
