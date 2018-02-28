#!/usr/bin/python

import sys
import time
import os

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def issueRequests(opts, urls, useSsl=False):
	print 'connecting to localhost:%s' % opts.httpdPort
	
	connection = runner.createConnection('localhost:' + opts.httpdPort, isHttps=useSsl)
	startTime = time.time();

	for url in urls:
		url = url.strip()
		print 'requesting url:', url
		runner.sendRequest(connection, url, opts.userId)
		runner.readResponse(connection, expectedHeaders={'result': runner.ACCESS_OK})
	
	return time.time() - startTime

def main():
	(opts, _) = optionParser.parseCommandLine()

	numUrls = 15000

	print "Overriding logLevel to INFO"
	opts.logLevel = 'INFO'

	# too many urls from alexa list go to cloud
	backendArgs = [
		'--setCurrentTime', '"2014-03-21 15:31:00"',
		'--disableWebrootCloud'
	]
	runner.startBackend(opts, processes, backendArgs);
	httpdArgs = []
	if opts.ssl:
		httpdArgs = ["-s", # use SSL
					"--keyFile", "~/ssl/httpd.key", # key file
					"--certFile", "~/ssl/httpd.pem" # certificate file
					]
	httpdArgs += ["--connectionPerIpLimitAbsolute", str(numUrls),
				  "--connectionPerIpLimitSlowDown", str(numUrls),
				]
	runner.startHttpd(opts, processes, httpdArgs)
	
	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:numUrls]
	
	duration = issueRequests(opts, urls, opts.ssl)
	performance = len(urls) / duration
	print 'reqs/s: %.1f (duration %.2f, urls %d)' % (performance, duration, len(urls))

	assert performance > 400, "Performance is lower than expected"
	
	output = None
	if opts.ssl:
		output = open("performance-ssl.txt", "w")
	else:
		output = open("performance.txt", "w")

	output.write('%.1f\n' % performance)
	output.close()
	
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
