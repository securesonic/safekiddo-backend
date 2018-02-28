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

	runner.loadSqlScript(opts, runner.getRepoDir() + '/sql/test_scenario3_inserts.sql')

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-04-10 18:01:00")
	issueRequest(opts, "http://www.onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)

	runner.issueSqlCommand(opts, "update access_control set end_time='19:00' where id=84")

	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, "update access_control set end_time='18:00' where id=84")

	issueRequest(opts, "http://www.onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)

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
