#!/usr/bin/python

# TODO: test scenario
import sys
import os
import time
import re

STATS_FILE = os.path.abspath('backend.stats')
processes = {}

LAST_VALUE_PATTERN = r"\[%s\] last value: %s"
MAXIMUM_VALUE_PATTERN = r"\[%s\] maximum value: %s"
DIFFERENCE_SINCE_LAST_PRINTING_PATTERN = r"\[%s\] difference since last printing: %s"

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

def checkRegexList(regexList):
	statsfile = open(STATS_FILE, 'r')
	for line in statsfile:
		for regex in regexList:
			compiledRegex = re.compile(regex)
			if compiledRegex.search(line) is not None:
				regexList.remove(regex)
	assert len(regexList) == 0, "some regexps didn't match: %s" % (regexList,)
	statsfile.close()
	
def main():
	(opts, _) = optionParser.parseCommandLine()

	try:
		os.remove(STATS_FILE)
	except OSError:
		pass

	runner.issueSqlCommand(opts, "insert into webroot_overrides (id, address, code_categories_ext_id) values (1, 'onet.pl', 10)")

	opts.statisticsPrintingInterval = 10 # seconds
	opts.statisticsFilePath = STATS_FILE

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-03-21 15:31:00")

	issueRequest(opts, "192.168.0.1", runner.ACCESS_OK)
	issueRequest(opts, "127.0.0.1", runner.ACCESS_OK)
	issueRequest(opts, "10.1.2.3", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.google.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.safekiddo.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.safekiddo.com", runner.ACCESS_OK)
	issueRequest(opts, "http://www.safekiddo.com", runner.ACCESS_OK)
	issueRequest(opts, "http://www.safekiddo.com", runner.ACCESS_OK)
	issueRequest(opts, "http://www.safekiddo.com", runner.ACCESS_OK)
	time.sleep(opts.statisticsPrintingInterval + 1)

	runner.stopBackend(processes)

	regexList = [
		LAST_VALUE_PATTERN % ("bcBcapMap", 2),
		MAXIMUM_VALUE_PATTERN % ("bcBcapMap", 2),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcBcapMap", 2),
		
		LAST_VALUE_PATTERN % ("bcRtuMap", ""),
		MAXIMUM_VALUE_PATTERN % ("bcRtuMap", ""),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcRtuMap", ""),
		
		LAST_VALUE_PATTERN % ("unset", ""),
		MAXIMUM_VALUE_PATTERN % ("unset", ""),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("unset", ""),
		
		LAST_VALUE_PATTERN % ("localIp", 3),
		MAXIMUM_VALUE_PATTERN % ("localIp", 3),		
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("localIp", 3),		
		
		LAST_VALUE_PATTERN % ("rtuCache", ""),		
		MAXIMUM_VALUE_PATTERN % ("rtuCache", ""),		
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("rtuCache", ""),		
		
		LAST_VALUE_PATTERN % ("localDb", 1),
		MAXIMUM_VALUE_PATTERN % ("localDb", 1),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("localDb", 1),
		
		LAST_VALUE_PATTERN % ("bcap", 2),
		MAXIMUM_VALUE_PATTERN % ("bcap", 2),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcap", 2),
		
		LAST_VALUE_PATTERN % ("bcapCache", 3),		
		MAXIMUM_VALUE_PATTERN % ("bcapCache", 3),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcapCache", 3),

		#Testing newValueSinceLastPrinting
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcBcapMap", 0),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcRtuMap", 0),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("unset", 0),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("localIp", 0),		
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("rtuCache", 0),		
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("localDb", 0),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcap", 0),
		DIFFERENCE_SINCE_LAST_PRINTING_PATTERN % ("bcapCache", 0),
	]
	
	checkRegexList(regexList)
		
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
