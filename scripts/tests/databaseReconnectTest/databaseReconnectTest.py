#!/usr/bin/python

# TODO: test scenario

import sys
import os
import subprocess

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueRequest(connection, userId, url, expectedResult):
	url = url.strip()
	print 'requesting url:', url
	runner.sendRequest(connection, url, userId)
	headers = runner.readResponse(connection, expectedHeaders = {'result': expectedResult})
	print 'Result:', headers['result']
	print 'all headers:\n', headers

def main():
	(opts, _) = optionParser.parseCommandLine()

	tunnelTargetPort = opts.dbPort
	tunnelTargetHost = opts.dbHost
	opts.dbPort = opts.tunnelPort
	opts.dbHost = 'localhost'

	runner.startTunnel(opts, processes, tunnelTargetHost, tunnelTargetPort)
	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-03-21 18:12:00")

	# we have to create connection here to be reused
	print 'connecting to db on localhost:%s' % opts.httpdPort
	connection = runner.createConnection('localhost:'+opts.httpdPort)
	issueRequest(connection, opts.userId, "http://www.onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)

	runner.stopTunnel(processes)
	issueRequest(connection, opts.userId, "http://www.onet.pl", runner.ACCESS_OK)

	runner.startTunnel(opts, processes, tunnelTargetHost, tunnelTargetPort)
	issueRequest(connection, opts.userId, "http://www.onet.pl", runner.INTERNET_ACCESS_FORBIDDEN)

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
