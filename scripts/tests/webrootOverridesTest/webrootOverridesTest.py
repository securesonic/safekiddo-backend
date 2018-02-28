#!/usr/bin/python

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
	
def insertIntoWebrootOverrides(opts, id, name, category):
	runner.issueSqlCommand(opts, "insert into webroot_overrides (id, address, code_categories_ext_id) values (%s, '%s', %s)" % (id, name, category))


def main():
	(opts, _) = optionParser.parseCommandLine()

	runner.startHttpd(opts, processes)

	runner.startBackend(opts, processes)
	runner.setBackendTime(processes, "2014-09-12 10:05:59")

	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "subdomain.some-obscene-site.com", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.abcD.com", runner.ACCESS_OK) # SAF-449

	runner.stopBackend(processes)

	#insert some unvalid URLs
	insertIntoWebrootOverrides(opts, 101, 'asd87asd9asdj@#$%^&*().pl', 11)
	insertIntoWebrootOverrides(opts, 102, '\/\/\/\/\/\/\////\///\\\\////\/\/', 11)
	insertIntoWebrootOverrides(opts, 104, 'onet', 11)

	insertIntoWebrootOverrides(opts, 105, 'www.Abcd.com', 11) # SAF-449
	#SAF-478 TEST
	insertIntoWebrootOverrides(opts, 106, "www.onet.pl/" + ('A' * 5060), 11)

	
	# verify backend loads overrides on startup
	insertIntoWebrootOverrides(opts, 1, 'onet.pl', 11)
	insertIntoWebrootOverrides(opts, 2, 'http://some-obscene-site.com', 11)

	runner.startBackend(opts, processes)
	runner.setBackendTime(processes, "2014-09-12 10:05:59")

	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "subdomain.some-obscene-site.com", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "www.abcD.com", runner.CATEGORY_BLOCKED_CUSTOM) # SAF-449

	runner.stopBackend(processes)

	# verify backend periodically reloads overrides from db
	overridesUpdateInterval = 10
	backendArgs = ['--overridesUpdateInterval', str(overridesUpdateInterval)]
	runner.startBackend(opts, processes, backendArgs)
	runner.setBackendTime(processes, "2014-09-12 10:05:59")

	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "subdomain.some-obscene-site.com", runner.CATEGORY_BLOCKED_CUSTOM)

	# site deleted from overrides
	runner.issueSqlCommand(opts, "delete from webroot_overrides where id=2")

	time.sleep(overridesUpdateInterval + 3)

	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "subdomain.some-obscene-site.com", runner.ACCESS_OK)

	# site added again to overrides
	insertIntoWebrootOverrides(opts, 3, 'http://some-obscene-site.com', 11)

	time.sleep(overridesUpdateInterval + 3)

	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "subdomain.some-obscene-site.com", runner.CATEGORY_BLOCKED_CUSTOM)

	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)

	insertIntoWebrootOverrides(opts, 4, 'http://bing.pl', 11)
	runner.reloadBackendOverrides(processes)

	issueRequest(opts, "onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.bing.pl", runner.CATEGORY_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, "delete from webroot_overrides")
	insertIntoWebrootOverrides(opts, 5, 'wp.pl', 11)
	runner.reloadBackendOverrides(processes)

	issueRequest(opts, "onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "www.sport.wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "www.wp.pl/?subpage", runner.CATEGORY_BLOCKED_CUSTOM)
	
	runner.issueSqlCommand(opts, "delete from webroot_overrides")
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
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
