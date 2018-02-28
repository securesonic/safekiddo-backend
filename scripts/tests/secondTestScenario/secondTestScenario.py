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

	runner.loadSqlScript(opts, runner.getRepoDir() + '/sql/test_scenario2_inserts.sql')

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-03-21 16:00:00")
	issueRequest(opts, "http://www.facebook.com", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.battle.net", runner.ACCESS_OK)

	runner.setBackendTime(processes, "2014-03-21 18:01:00")
	issueRequest(opts, "http://www.battle.net", runner.INTERNET_ACCESS_FORBIDDEN)

	runner.loadSqlScript(opts, runner.getRepoDir() + '/sql/test_scenario2_parent_inserts.sql')

	runner.setBackendTime(processes, "2014-03-21 18:02:00")
	issueRequest(opts, "http://www.facebook.com", runner.INTERNET_ACCESS_FORBIDDEN)

	runner.setBackendTime(processes, "2014-03-22 08:00:00")
	issueRequest(opts, "http://www.facebook.com", runner.ACCESS_OK)
	issueRequest(opts, "http://www.battle.net", runner.CATEGORY_BLOCKED_CUSTOM)

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
