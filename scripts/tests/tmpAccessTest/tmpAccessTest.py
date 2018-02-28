#!/usr/bin/python

import sys
import os
import subprocess

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

	currentTime = "2014-03-21 20:11:00"
	runner.setBackendTime(processes, currentTime)
	issueRequest(opts, "http://onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)
	accessStartTime = "2014-03-21 20:00:00"
	sqlStatement = 'insert into tmp_access_control (id, code_child_id, start_date, duration, parent_accepted)'\
	" values (1, 37, '%s', '02:12:00', false)" % accessStartTime
	runner.issueSqlCommand(opts, sqlStatement)
	issueRequest(opts, "http://onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)
	sqlStatement = 'update tmp_access_control set parent_accepted=true where id=1'
	runner.issueSqlCommand(opts, sqlStatement)
	issueRequest(opts, "http://onet.pl", runner.ACCESS_OK)

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
