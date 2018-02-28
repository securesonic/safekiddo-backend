#!/usr/bin/python

# TODO: test scenario

import sys
import time
import datetime
import os

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser, mgmtLib

TEST_FILE = 'testAdmissionControlCustomSettingsFile'

def issueRequest(opts, url, expectedResult, expectedHttpStatus = 200, userId = None, headerResult = runner.ACCESS_OK, addHeaders = {}):
	connection = runner.createConnection('localhost:' + opts.httpdPort)
	url = url.strip()
	if userId is None:
		userId = opts.userId
	runner.sendRequest(connection, url, userId, addHeaders = addHeaders)
	runner.readResponse(connection, expectedHeaders={'result': headerResult}, expectedHttpStatus = expectedHttpStatus)

def reloadConfigFile(processes, fileContent):
	admissionControlCustomSettingsFile = open(TEST_FILE, 'w')
	admissionControlCustomSettingsFile.write(fileContent)
	admissionControlCustomSettingsFile.close()
	runner.reloadHttpdAdmissionControlCustomSettingsFile(processes)

def runHttpd(opts, processes, fileContent, shouldFail, stop = True, limitAbsolute = 600, limitSlowDown = 600, slowDownDuration = 300):
	admissionControlCustomSettingsFile = open(TEST_FILE, 'w')
	admissionControlCustomSettingsFile.write(fileContent)
	admissionControlCustomSettingsFile.close()
	httpdArgs = [	"--connectionPerIpLimitAbsolute", str(limitAbsolute),
					"--connectionPerIpLimitSlowDown", str(limitSlowDown),
					"--slowDownDuration", str(slowDownDuration),
				]
	#ERROR - two times same line in file
	time.sleep(2)
	runner.startHttpd(opts, processes, httpdArgs, shouldFail, createAdmissionControlFileIfNotExists = False)
	if not shouldFail and stop:
		runner.stopHttpd(processes)

def main():
	(opts, _) = optionParser.parseCommandLine()
	defaultAdmissionControlCustomSettingsFile = opts.admissionControlCustomSettingsFile
	opts.admissionControlCustomSettingsFile = TEST_FILE

	backendArgs = ['--disableWebrootCloud']
	runner.startBackend(opts, processes, backendArgs)
	runner.setBackendTime(processes, "2014-09-09 16:42:00")
	
	#BEGIN OF: custom setting file tests
	#First case - not exiting file [ERROR]
	httpdArgs = [	"--connectionPerIpLimitAbsolute", "600",
					"--connectionPerIpLimitSlowDown", "600",
					"--slowDownDuration", "300",
				]
	runner.startHttpd(opts, processes, httpdArgs, True, createAdmissionControlFileIfNotExists = False)
	
	#Second case - exiting and empty file [ok]
	admissionControlCustomSettingsFile = open(TEST_FILE, 'w')
	admissionControlCustomSettingsFile.close()
	httpdArgs = [	"--connectionPerIpLimitAbsolute", "600",
					"--connectionPerIpLimitSlowDown", "600",
					"--slowDownDuration", "300",
				]
	runner.startHttpd(opts, processes, httpdArgs, createAdmissionControlFileIfNotExists = False)
	runner.stopHttpd(processes)
	
	#Working file
	runHttpd(
		opts, processes,
		"168.192.0.1 123 543\n" + 
		"127.3.0.1 123 543\n" + 
		"91.32.111.15 123 543\n" + 
		"168.192.0.4 86 153\n",
		False
	)
	
	#Third case - file parsing errors
	#2x same ip 
	runHttpd(
		opts, processes,
		"127.0.0.1 123 543\n" + 
		"127.3.0.1 123 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.1 86 153\n",
		True
	)
	
	#2x same ip 
	runHttpd(
		opts, processes,
		"2001:0db8:0000:0000:0000:ff00:0042:8329 123 543\n" + 
		"2001:db8:0:0:0:ff00:42:8329 123 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.1 86 153\n",
		True
	)
	
	#2x same ip 
	runHttpd(
		opts, processes,
		"2001:db8:0:0:0:ff00:42:8329\n 100 153" + 
		"2001:db8::ff00:42:8329\n 86 153" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.1 86 153\n",
		True
	)
	
	#2x same ip 
	runHttpd(
		opts, processes,
		"2001:db8:0:0:0:ff00:42:8329 86 153\n" + 
		"0:0:0:0:0:ffff:630c:224c 86 153\n" + 
		"127.1.0.5 123 543\n" + 
		"99.12.34.76 86 153\n",
		True
	)
	
	#Parsing error (first line)
	runHttpd(
		opts, processes,
		"1 127.0.0.1 543\n" + 
		"127.3.0.1 123 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.2 86 153\n",
		True
	)
	
	#Parsing error (first line)
	runHttpd(
		opts, processes,
		"1 127.0.0.1 123 543\n" + 
		"127.3.0.1 123 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.2 86 153\n",
		True
	)
	
	#Parsing error (third line)
	runHttpd(
		opts, processes,
		"127.0.0.1 123 543\n" + 
		"127.3.0.1 123 543\n" + 
		"127.1.0.5 123 543 \n" + 
		"127.0.0.2 86 153\n",
		True
	)
	
	#Parsing error (third line)
	runHttpd(
		opts, processes,
		"127.0.0.1 123 543\n" + 
		"127.3.0.1 123 543\n" + 
		"127.1.0.5 12 123 543\n" + 
		"127.0.0.2 86 153\n",
		True
	)
	
	#Parsing error (second line)
	runHttpd(
		opts, processes,
		"127.0.0.1 123 543\n" + 
		"127.0.1 123 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.2 86 153\n",
		True
	)
	
	#Parsing error (second line)
	runHttpd(
		opts, processes,
		"127.0.0.1 123 543\n" + 
		"123 127.3.0.1 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.2 86 153\n",
		True
	)
	
	#Parsing error (second line)
	runHttpd(
		opts, processes,
		"127.0.0.1 123 543\n" + 
		"127.3.0.1\n",
		True
	)
	
	#Fourth case - connectionPerIpLimitAbsolute
	
	runHttpd(
		opts, processes,
		"127.3.0.1 123 543\n" + 
		"91.32.111.15 123 543\n" + 
		"127.0.0.1 10 1000\n" + 
		"168.192.0.4 86 153\n",
		False,
		False
	)
	
	for i in range(0, 10):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503)

	runner.stopHttpd(processes)
	
	#Fifth case - connectionPerIpLimitAbsolute
	
	runHttpd(
		opts, processes,
		"127.3.0.1 123 543\n" + 
		"91.32.111.15 123 543\n" + 
		"127.0.0.1 100 5\n" + 
		"168.192.0.4 86 153\n",
		False,
		False
	)
	
	startTime = datetime.datetime.now()
	for i in range(0, 50):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	assert(datetime.datetime.now() - startTime > datetime.timedelta(seconds=10))
	
	runner.stopHttpd(processes)
	
	#Sixth case - file reloading: file is ok
	
	runHttpd(
		opts, processes,
		"127.3.0.1 123 543\n" + 
		"91.32.111.15 123 543\n" + 
		"127.0.0.1 10 1000\n" + 
		"168.192.0.4 86 153\n",
		False,
		False
	)
	
	for i in range(0, 10):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503)
	
	reloadConfigFile(processes, "127.0.0.1 100 1000\n")

	for i in range(0, 30):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)

	runner.stopHttpd(processes)
	
	#Seventh case - file reloading: file with error, but httpd don't crash,
	#in the end loading correct file
	
	runHttpd(
		opts, processes,
		"127.3.0.1 123 543\n" + 
		"91.32.111.15 123 543\n" + 
		"127.0.0.1 10 1000\n" + 
		"168.192.0.4 86 153\n",
		False,
		False
	)
	
	for i in range(0, 10):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503)
	
	reloadConfigFile(processes, "asdfasdfq345\n\n2345sadf \n")
	
	reloadConfigFile(
		processes, 
		"127.0.0.1 123 543\n" + 
		"123 127.3.0.1 543\n" + 
		"127.1.0.5 123 543\n" + 
		"127.0.0.2 86 153\n"
	)
	
	reloadConfigFile(processes, "127.0.0.1 100 1000\n")
	
	reloadConfigFile(
		processes, 
		"2001:db8:0:0:0:ff00:42:8329 86 153\n" + 
		"0:0:0:0:0:ffff:630c:224c 86 153\n" + 
		"127.1.0.5 123 543\n" + 
		"99.12.34.76 86 153\n"
	)
	
	reloadConfigFile(processes, "127.0.0.1 100 1000\n")
	reloadConfigFile(processes, "127.0.0.1 100 1000\n")
	reloadConfigFile(processes, "127.0.0.1 100 1000\n")
	
	for i in range(0, 30):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)

	runner.stopHttpd(processes)

	#END OF: custom setting file tests

	# X-Forwarded-For test: don't use the header if not enabled
	assert opts.useForwardedFor == False, "in tests we don't use X-Forwarded-For by default"
	runHttpd(
		opts, processes,
		"127.0.0.42 20 1000\n",
		False,
		False,
		limitAbsolute = 10
	)

	addHeaders = {'X-Forwarded-For': '127.0.0.42'}
	# header is ignored by httpd
	for i in range(10):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, addHeaders = addHeaders)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503, addHeaders = addHeaders)

	runner.stopHttpd(processes)

	# X-Forwarded-For test: use the header if enabled
	opts.useForwardedFor = True
	runHttpd(
		opts, processes,
		"127.0.0.42 20 1000\n",
		False,
		False,
		limitAbsolute = 10
	)

	addHeaders = {'X-Forwarded-For': '127.0.0.42'}
	# more requests possible for this IP
	for i in range(20):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, addHeaders = addHeaders)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503, addHeaders = addHeaders)

	# X-Forwarded-For test: whitelisted IP is itself a proxy
	# connection goes like this: 200.50.10.5 -> 127.0.0.42 -> load balancer -> httpd
	addHeaders = {'X-Forwarded-For': '200.50.10.5, 127.0.0.42'}
	# renew limits
	reloadConfigFile(processes, "127.0.0.42 20 1000\n")
	for i in range(20):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, addHeaders = addHeaders)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503, addHeaders = addHeaders)

	# X-Forwarded-For test: unauthorized user added the header for whitelisted IP, load balancer added real attacker IP
	addHeaders = {'X-Forwarded-For': '127.0.0.42, 100.100.100.100'}
	# don't renew limits - there where only requests from whitelisted IP
	# only 10 requests possible
	for i in range(10):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, addHeaders = addHeaders)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503, addHeaders = addHeaders)

	runner.stopHttpd(processes)
	opts.useForwardedFor = False

	#END OF: X-Forwarded-For tests

	opts.admissionControlCustomSettingsFile = defaultAdmissionControlCustomSettingsFile

	httpdArgs = [	"--connectionPerIpLimitAbsolute", "600",
					"--connectionPerIpLimitSlowDown", "600",
					"--slowDownDuration", "300",
				]
	runner.startHttpd(opts, processes, httpdArgs)
	
	startTime = datetime.datetime.now();
	for i in range(0, 500):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	assert(datetime.datetime.now() - startTime < datetime.timedelta(seconds=10))
	
	runner.stopHttpd(processes)
	time.sleep(5)
	httpdArgs = [	"--connectionPerIpLimitAbsolute", "600",
					"--connectionPerIpLimitSlowDown", "100",
					"--slowDownDuration", "300"
				]
	runner.startHttpd(opts, processes, httpdArgs)
	
	startTime = datetime.datetime.now();
	for i in range(0, 150):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	assert(datetime.datetime.now() - startTime > datetime.timedelta(seconds=10))
	
	runner.stopHttpd(processes)
	time.sleep(5)
	httpdArgs = [	"--connectionPerIpLimitAbsolute", "105",
					"--connectionPerIpLimitSlowDown", "103",
					"--slowDownDuration", "300"
				]
	runner.startHttpd(opts, processes, httpdArgs)
	
	startTime = datetime.datetime.now();
	for i in range(0, 105):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503)

	runner.stopHttpd(processes)
	time.sleep(5)
	httpdArgs = [	"--connectionPerIpLimitAbsolute", "1000",
					"--connectionPerIpLimitSlowDown", "900",
					"--slowDownDuration", "300"
				]
	runner.startHttpd(opts, processes, httpdArgs)
	
	startTime = datetime.datetime.now();
	for i in range(0, 20):
		issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, userId = "this444User333Definitly222Not111Exist12345", headerResult = runner.UNKNOWN_USER)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK, 503)

	
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
