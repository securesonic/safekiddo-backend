#!/usr/bin/python

# TODO: test scenario

import sys
import os

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueRequest(opts, url, expectedResult, userId):
	print 'connecting to localhost:%s' % opts.httpdPort

	connection = runner.createConnection('localhost:' + opts.httpdPort)

	url = url.strip()
	print 'requesting url:', url
	runner.sendRequest(connection, url, userId)
	headers = runner.readResponse(connection, expectedHeaders = {'result': expectedResult})
	print 'Result:', headers['result']
	print 'all headers:\n', headers


def main():
	(opts, _) = optionParser.parseCommandLine()

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-03-24 12:00:01")
	issueRequest(opts, "http://onet.pl", runner.ACCESS_OK, '70514')
	issueRequest(opts, "http://onet.pl", runner.UNKNOWN_USER, '12345123123123')

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
