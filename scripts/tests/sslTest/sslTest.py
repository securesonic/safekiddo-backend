#!/usr/bin/python

# TODO: test scenario

import sys
import os

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueSslRequest(opts, url, timeout):
	print 'connecting to localhost:%s' % opts.httpdPort
	
	connection = runner.createConnection('localhost:' + opts.httpdPort, isHttps=True)

	url = url.strip()
	print 'requesting url:', url
	runner.sendRequest(connection, url, opts.userId)
	headers = runner.readResponse(connection, timeoutExpected=timeout)
	print 'Result:', headers['result']
	print 'all headers:\n', headers


def main():
	(opts, _) = optionParser.parseCommandLine()
	
	httpdArgs = [
				"-s", # use SSL
				"--keyFile", "~/ssl/httpd.key", # key file
				"--certFile", "~/ssl/httpd.pem" # certificate file
				]
	
	runner.startHttpd(opts, processes, httpdArgs)
	
	issueSslRequest(opts, "http://www.onet.pl", True)
		
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
