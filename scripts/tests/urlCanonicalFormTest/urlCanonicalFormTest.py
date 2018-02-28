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
	runner.sendRequest(connection, url, opts.userId, quote = False)
	headers = runner.readResponse(connection, expectedHeaders = {'result': expectedResult})
	print 'Result:', headers['result']
	print 'all headers:\n', headers


def toDbBool(value):
	if value:
		return 'true'
	return 'false'

def makeAllSubdomainsInsert(urlPatternId, hostName):
	assert '/' not in hostName, "hostName must not contain '/': %s" % hostName
	return "insert into url (id, address) values (%d, '%s')" % (urlPatternId, hostName)

def makeDomainWithPathInsert(urlPatternId, hostName, path):
	assert '/' not in hostName, "hostName must not contain '/': %s" % hostName
	assert path[0] == '/', "path must start with '/': %s" % path
	return "insert into url (id, address) values (%d, '%s')" % (urlPatternId, hostName + path)

def makeProfileUrlDecision(urlPatternId, blocked, profileId = 20):
	return "insert into profile_url_list (code_profile_id, code_url_id, blocked) values (%d, %d, %s)" % (profileId, urlPatternId,
		toDbBool(blocked))

def makeDeletes(profileId = 20):
	return "delete from profile_url_list where code_profile_id=%d; delete from url" % profileId

def main():
	(opts, _) = optionParser.parseCommandLine()

	runner.loadSqlScript(opts, runner.getRepoDir() + '/sql/test_scenario1_inserts.sql')

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)

	runner.setBackendTime(processes, "2014-04-10 08:00:00")
	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl/abc", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl/abc:123", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl/abc:123?../../..", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl/..", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl/../wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl/../wp.pl:123", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl:123/../wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl:123/?../wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl:123/?../?wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "ONET.PL/", runner.ACCESS_OK)
	issueRequest(opts, "ONET.PL", runner.ACCESS_OK)
	issueRequest(opts, "HTTP://ONET.PL", runner.ACCESS_OK)
	issueRequest(opts, "HTTP://ONET.PL/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "onet.pl"),
		makeProfileUrlDecision(1, True)
		]))

	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc:123", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc:123?../../..", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/..", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/../wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/../wp.pl:123", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/../wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/?../wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/?../?wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "ONET.PL/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "ONET.PL", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "HTTP://ONET.PL", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "HTTP://ONET.PL/", runner.URL_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		"insert into profile_categories_groups_list (id, code_profile_id, code_categories_groups_id, blocked) values \
(2, 20, 46, true)" # (46,'Portale');
		]))

	issueRequest(opts, "onet.pl/", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc:123", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc:123?../../..", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/..", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/../wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/../wp.pl:123", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/../wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/?../wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/?../?wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "ONET.PL/", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "ONET.PL", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "HTTP://ONET.PL", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "HTTP://ONET.PL/", runner.CATEGORY_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		"delete from profile_categories_groups_list where id=2",
		makeDomainWithPathInsert(1, "onet.pl", "/"),
		makeProfileUrlDecision(1, True)
		]))

	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc:123", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/abc:123?../../..", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/..", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/../wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/../wp.pl:123", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/../wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/?../wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl:123/?../?wp.pl", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "ONET.PL/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "ONET.PL", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "HTTP://ONET.PL", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "HTTP://ONET.PL/", runner.URL_BLOCKED_CUSTOM)

	issueRequest(opts, "HtTP://OnET.PL/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/%2E%2E/wp.pl", runner.URL_BLOCKED_CUSTOM)

	# malformed urls
	issueRequest(opts, "http://", runner.ACCESS_OK)
	issueRequest(opts, "http:/", runner.ACCESS_OK)
	issueRequest(opts, ":/", runner.ACCESS_OK)
	issueRequest(opts, "://", runner.ACCESS_OK)
	issueRequest(opts, "/", runner.ACCESS_OK)
	issueRequest(opts, ":", runner.ACCESS_OK)
	issueRequest(opts, "?", runner.ACCESS_OK)
	issueRequest(opts, "abc/", runner.ACCESS_OK)
	issueRequest(opts, "abc/%00/aaa", runner.ACCESS_OK)
	issueRequest(opts, "abc/%0/", runner.ACCESS_OK)
	issueRequest(opts, "abc/%0", runner.ACCESS_OK)
	issueRequest(opts, "abc/%", runner.ACCESS_OK)
	issueRequest(opts, "abc/%ff", runner.ACCESS_OK) # not malformed but corner-case
	issueRequest(opts, "abc/%g0", runner.ACCESS_OK)
	issueRequest(opts, "abc/%0g", runner.ACCESS_OK)
	issueRequest(opts, "abc/%G0", runner.ACCESS_OK)
	issueRequest(opts, "abc/%0G", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "onet.pl", "/abc"),
		makeProfileUrlDecision(1, True)
		]))

	issueRequest(opts, "http://onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl/abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/xyz/../abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl//abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/./abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/../abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/Abc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/%41bc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/%61bc", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "http://onet.pl/%42bc", runner.ACCESS_OK)
	issueRequest(opts, "http://onet.pl/%62bc", runner.ACCESS_OK)

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
