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

	runner.startBackend(opts, processes)
	runner.startHttpd(opts, processes)
	runner.setBackendTime(processes, "2014-04-10 08:00:00")

	#SAF-478 TEST
	issueRequest(opts, "www.onet.pl/" + ('A' * 5060), runner.ACCESS_OK)

	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "wp.pl/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "onet.pl"),
		makeProfileUrlDecision(1, True)
		]))

	# SAF-440 (error with google.pl?safesearch=on) test
	issueRequest(opts, "onet.pl?", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl?/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/?", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl?/?", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl?safesearch", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/?safesearch", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl?/safesearch", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl?/?safesearch", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl?safesearch", runner.URL_BLOCKED_CUSTOM)
	# similar bug but with handling of '#' character
	issueRequest(opts, "onet.pl#top", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl#/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "onet.pl/#bottom", runner.URL_BLOCKED_CUSTOM)

	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "wp.pl/", runner.ACCESS_OK)
	issueRequest(opts, "m.onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "xxxxxxxxxxxxonet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "net.pl/", runner.ACCESS_OK)
	issueRequest(opts, ".pl/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "onet.pl"),
		makeAllSubdomainsInsert(2, "www.onet.pl"),
		makeProfileUrlDecision(1, True),
		makeProfileUrlDecision(2, False)
		]))

	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK) # matched by both, longer is more specific

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "www.onet.pl"),
		makeAllSubdomainsInsert(2, "onet.pl"),
		makeProfileUrlDecision(1, False),
		makeProfileUrlDecision(2, True)
		]))

	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK) # matched by both, longer is more specific

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/"),
		makeProfileUrlDecision(1, True)
		]))

	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/xxx", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "x.www.onet.pl/xxx", runner.ACCESS_OK)
	issueRequest(opts, ".www.onet.pl/xxx", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/abc/def"),
		makeProfileUrlDecision(1, True)
		]))

	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/xxx", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/abc/def", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/abc/defghi", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/abc/def/ghi", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "x.www.onet.pl/abc/def/ghi", runner.ACCESS_OK)
	issueRequest(opts, "xwww.onet.pl/abc/def/ghi", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "www.onet.pl"),
		makeDomainWithPathInsert(2, "www.onet.pl", "/"),
		makeProfileUrlDecision(1, True),
		makeProfileUrlDecision(2, False)
		]))

	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK) # matched by both, domain with path is more specific
	issueRequest(opts, "abcwww.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "xyz.www.onet.pl/", runner.URL_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/"),
		makeAllSubdomainsInsert(2, "www.onet.pl"),
		makeProfileUrlDecision(1, False),
		makeProfileUrlDecision(2, True)
		]))

	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK) # matched by both, domain with path is more specific
	issueRequest(opts, "abcwww.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "xyz.www.onet.pl/", runner.URL_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "www.onet.pl"),
		makeDomainWithPathInsert(2, "www.onet.pl", "/some/path"),
		makeProfileUrlDecision(1, True),
		makeProfileUrlDecision(2, False)
		]))

	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "xyz.www.onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/path", runner.ACCESS_OK)
	issueRequest(opts, "xyz.www.onet.pl/some/path", runner.URL_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/some/p"),
		makeDomainWithPathInsert(2, "www.onet.pl", "/some/path"),
		makeProfileUrlDecision(1, True),
		makeProfileUrlDecision(2, False)
		]))

	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/some/p", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/pa", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/pat", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/path", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/some/pathx", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/some/path/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/some/path"),
		makeDomainWithPathInsert(2, "www.onet.pl", "/some/p"),
		makeProfileUrlDecision(1, False),
		makeProfileUrlDecision(2, True)
		]))

	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/some/p", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/pa", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/pat", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/path", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/some/pathx", runner.ACCESS_OK)
	issueRequest(opts, "www.onet.pl/some/path/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeAllSubdomainsInsert(1, "onet.pl"),
		makeDomainWithPathInsert(2, "www.onet.pl", "/some/"),
		makeDomainWithPathInsert(3, "www.onet.pl", "/"),
		makeProfileUrlDecision(1, True),
		makeProfileUrlDecision(2, False),
		makeProfileUrlDecision(3, True)
		]))

	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/some/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/"),
		makeDomainWithPathInsert(2, "onet.pl", "/"),
		makeProfileUrlDecision(1, False),
		makeProfileUrlDecision(2, True)
		]))

	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "asdasdadasasdasddas.www.onet.pl/", runner.ACCESS_OK)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		makeDomainWithPathInsert(1, "www.onet.pl", "/"),
		makeDomainWithPathInsert(2, "onet.pl", "/"),
		makeAllSubdomainsInsert(3, "www.onet.pl"),
		makeProfileUrlDecision(1, False),
		makeProfileUrlDecision(2, True),
		makeProfileUrlDecision(3, True)
		]))

	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM)
	issueRequest(opts, "www.onet.pl/", runner.ACCESS_OK)
	issueRequest(opts, "asdasdadasasdasddas.www.onet.pl/", runner.URL_BLOCKED_CUSTOM)

	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		"insert into profile_categories_groups_list (id, code_profile_id, code_categories_groups_id, blocked) values \
(2, 20, 46, true)" # (46,'Portale');
		]))
	issueRequest(opts, "onet.pl/", runner.CATEGORY_BLOCKED_CUSTOM)
	runner.issueSqlCommand(opts, ';'.join([
		makeAllSubdomainsInsert(1, "onet.pl"),
		makeProfileUrlDecision(1, False)
		]))
	issueRequest(opts, "onet.pl/", runner.ACCESS_OK) # url pattern allow decision overrides category group deny decision
	runner.issueSqlCommand(opts, ';'.join([
		makeDeletes(),
		"update profile_categories_groups_list set blocked=false where id=2"
		]))
	issueRequest(opts, "onet.pl/", runner.ACCESS_OK)
	runner.issueSqlCommand(opts, ';'.join([
		makeAllSubdomainsInsert(1, "onet.pl"),
		makeProfileUrlDecision(1, True)
		]))
	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM) # url pattern deny decision overrides category group allow decision
	runner.issueSqlCommand(opts, ';'.join([
		"update profile_categories_groups_list set blocked=true where id=2"
		]))
	issueRequest(opts, "onet.pl/", runner.URL_BLOCKED_CUSTOM) # still URL_BLOCKED_CUSTOM although also blocked by category group

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
