#!/usr/bin/python

# TODO: test scenario

import sys
import os
import copy
import datetime
import psycopg2

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser
from library.runner import UserActionCode

def total_seconds(td):
	return (td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6) / 10**6

def computeUtcOffset(dt):
	t = total_seconds(dt - datetime.datetime(1970, 1, 1))
	loc = datetime.datetime.fromtimestamp(t)
	utc = datetime.datetime.utcfromtimestamp(t)
	return loc - utc

def isInt(value):
	try:
		_ = int(value)
	except ValueError:
		return False
	return True

def isValidUserAction(userAction):
	if userAction is None:
		return True
	if not isInt(userAction):
		return False
	return userAction == UserActionCode.NON_INTENTIONAL_REQUEST or \
		userAction == UserActionCode.INTENTIONAL_REQUEST or \
		userAction == UserActionCode.CATEGORY_GROUP_QUERY

class Request:
	def __init__(self, url, expectedResult, childTimestamp, userId, categoryGroupId, userAction = None,
			quote = True):
		self.url = url
		self.expectedResult = expectedResult
		self.childTimestamp = childTimestamp
		self.userId = userId
		self.categoryGroupId = categoryGroupId
		self.userAction = userAction
		self.quote = quote
		if self.quote:
			self.urlInDb = runner.quoteUrl(self.url)
		else:
			self.urlInDb = self.url

	def toDbFormat(self):
		if not (isValidUserAction(self.userAction) and (self.userAction is None or
				self.userAction == UserActionCode.INTENTIONAL_REQUEST)):
			# this request is not logged to db
			return None
		# categoryGroupId may still be None in case of ACCESS_OK, but in logs db it may be not None
		categoryGroupId = None
		if self.categoryGroupId is not None:
			categoryGroupId = int(self.categoryGroupId)
		serverTimestamp = self.childTimestamp - computeUtcOffset(self.childTimestamp)
		return (
			# server_timestamp timestamp with time zone not null,
			serverTimestamp,
			# child_timestamp timestamp not null,
			self.childTimestamp,
			# url text not null,
			self.urlInDb,
			# child_uuid text not null,
			self.userId,
			# response_code integer not null,
			int(self.expectedResult),
			# category_group_id integer
			categoryGroupId,
		)


def issueConcurrentRequests(opts, requests):
	print 'connecting to localhost:%s' % opts.httpdPort

	pool = []
	for _ in xrange(len(requests)):
		pool.append(runner.createConnection('localhost:' + opts.httpdPort))

	for idx in xrange(len(requests)):
		connection = pool[idx]
		request = requests[idx]
		print 'requesting url:', request.url
		runner.sendRequest(connection, request.url, request.userId, userAction = request.userAction,
			quote = request.quote)

	for idx in xrange(len(requests)):
		connection = pool[idx]
		request = requests[idx]

		expectedHeaders = dict()
		if request.expectedResult is not None:
			expectedHeaders['result'] = request.expectedResult
		if request.categoryGroupId is not None and (
				# SAF-382
				request.expectedResult != runner.ACCESS_OK or
				request.userAction == UserActionCode.CATEGORY_GROUP_QUERY):
			expectedHeaders['category-group-id'] = request.categoryGroupId

		print 'Request:', request.url
		if not isValidUserAction(request.userAction):
			print 'Sent invalid UserAction (%s), expecting HTTP 400' % (request.userAction,)
			runner.readResponse(connection, expectedHttpStatus = 400)
		else:
			headers = runner.readResponse(connection, expectedHeaders)
			print 'Result:', headers['result']
			print 'all headers:\n', headers

			if request.expectedResult is None:
				request.expectedResult = headers['result']
			if request.categoryGroupId is None and 'category-group-id' in headers:
				request.categoryGroupId = headers['category-group-id']


def requestMatches(request, record):
	for requestValue, recordValue in zip(request, record):
		if requestValue is not None and recordValue != requestValue:
			return False
	return True

def checkLogs(opts, allRequests):
	params = copy.copy(opts.__dict__) # what a hack

	for nameLogs, name in [('dbHostLogs', 'dbHost'), ('dbPortLogs', 'dbPort')]:
		if params[nameLogs] is None:
			params[nameLogs] = params[name]

	conn_params = []
	for name1, name2 in [('host', 'dbHostLogs'), ('port', 'dbPortLogs'), ('dbname', 'dbNameLogs'), ('user', 'dbUser'), ('password', 'dbPassword')]:
		if params[name2]:
			conn_params.append("%s='%s'" % (name1, params[name2]))
	conn_string = ' '.join(conn_params)
	print "Connecting to database\n->%s" % (conn_string)
	conn = psycopg2.connect(conn_string)
	cursor = conn.cursor()
	columns = "server_timestamp, child_timestamp, url, child_uuid, response_code, category_group_id"
	cursor.execute("select %s from request_log" % columns)
	records = cursor.fetchall()

	checksOk = True
	expectedDb = map(Request.toDbFormat, allRequests)
	expectedDb = filter(None, expectedDb) # remove None elements
	if len(records) != len(expectedDb):
		print "Error: there are %d requests in logs db but should be %d" % (len(records), len(expectedDb))
		print "Expected db contents (%s):\n%s" % (columns, "\n".join(map(str, expectedDb)))
		checksOk = False

	if checksOk:
		records.sort()
		expectedDb.sort()
		for i in xrange(len(records)):
			if not requestMatches(expectedDb[i], records[i]):
				print "Error: request %s not found, instead got %s" % (expectedDb[i], records[i])
				checksOk = False
				break

	assert checksOk, "Some checks failed"

def main():
	(opts, _) = optionParser.parseCommandLine()

	if opts.dbHost == "" or opts.dbHost == "localhost":
		print "db is local"
		NUM_REQUESTS = 1000
	else:
		print "db is remote"
		NUM_REQUESTS = 20

	requestLimit = str(NUM_REQUESTS + 200)
	httpdArgs = ['--connectionLimit', requestLimit,
				 "--connectionPerIpLimitAbsolute", requestLimit,
				 "--connectionPerIpLimitSlowDown", requestLimit
				]
	runner.startHttpd(opts, processes, httpdArgs)
	runner.startBackend(opts, processes)

	allRequests = []

	# start backend at 16:00
	timestamp = datetime.datetime.combine(datetime.date.today(), datetime.time(16, 0))
	runner.setBackendTime(processes, timestamp)

	curRequests = []
	curRequests.append(Request("http://www.facebook.com", runner.CATEGORY_BLOCKED_CUSTOM, timestamp, opts.userId, '3'))
	curRequests.append(Request("http://www.battle.net", runner.ACCESS_OK, timestamp, opts.userId, '2'))
	# The same second time. This time let's don't verify the categoryGroupId in response. It is however verified that whatever was the
	# category-group-id header in response, the same goes to request log.
	curRequests.append(Request("http://www.facebook.com", runner.CATEGORY_BLOCKED_CUSTOM, timestamp, opts.userId, None))
	curRequests.append(Request("http://www.battle.net", runner.ACCESS_OK, timestamp, opts.userId, None))

	# verify special characters are properly logged
	curRequests.append(Request("http://www.battle.net/abc\ndef", runner.ACCESS_OK, timestamp, opts.userId, None))
	curRequests.append(Request("http://www.battle.net/abc\tdef", runner.ACCESS_OK, timestamp, opts.userId, None))
	curRequests.append(Request("http://www.battle.net/abc\\def", runner.ACCESS_OK, timestamp, opts.userId, None))

	# verify not-empty UserAction header
	curRequests.append(Request("http://www.battle.net", runner.ACCESS_OK, timestamp, opts.userId, None, userAction =
		UserActionCode.NON_INTENTIONAL_REQUEST))
	curRequests.append(Request("http://www.battle.net", runner.ACCESS_OK, timestamp, opts.userId, None, userAction =
		UserActionCode.INTENTIONAL_REQUEST))
	curRequests.append(Request("http://www.battle.net", runner.ACCESS_OK, timestamp, opts.userId, None, userAction =
		UserActionCode.CATEGORY_GROUP_QUERY))
	# malformed header
	curRequests.append(Request("www.google.com", runner.ACCESS_OK, timestamp, opts.userId, None, userAction = 'abc'))
	curRequests.append(Request("www.google.com", runner.ACCESS_OK, timestamp, opts.userId, None, userAction = 3))
	curRequests.append(Request("www.google.com", runner.ACCESS_OK, timestamp, opts.userId, None, userAction = -1))

	# verify category group query works
	curRequests.append(Request("www.sklep-intymny.pl", runner.CATEGORY_BLOCKED_CUSTOM, timestamp, opts.userId, '1', userAction =
		UserActionCode.CATEGORY_GROUP_QUERY))
	# verifies that category group is returned even for not blocked url
	curRequests.append(Request("www.onet.pl", runner.ACCESS_OK, timestamp, opts.userId, '6', userAction =
		UserActionCode.CATEGORY_GROUP_QUERY))
	# test for SAF-351
	curRequests.append(Request("http://www.some-uncategorized-site.com", runner.ACCESS_OK, timestamp, opts.userId, None, userAction =
		UserActionCode.CATEGORY_GROUP_QUERY))
	# test for SAF-222
	curRequests.append(Request("http://www.some-uncategorized-site.com", runner.ACCESS_OK, timestamp, opts.userId, '27'))
	curRequests.append(Request("http://192.168.1.1", runner.ACCESS_OK, timestamp, opts.userId, '27'))

	# verify characters outside of ascii are properly handled
	request = Request("www.google.com/\x80abc\xaadef\xffghi", runner.ACCESS_OK, timestamp, opts.userId, None, quote = False)
	request.urlInDb = "www.google.com/?abc?def?ghi"
	curRequests.append(request)

	allRequests.extend(curRequests)
	issueConcurrentRequests(opts, curRequests)

	# start backend at 18:01
	timestamp = datetime.datetime.combine(datetime.date.today(), datetime.time(18, 1))
	runner.setBackendTime(processes, timestamp)

	curRequests = []
	curRequests.append(Request("http://www.battle.net", runner.INTERNET_ACCESS_FORBIDDEN, timestamp, opts.userId, '2'))

	allRequests.extend(curRequests)
	issueConcurrentRequests(opts, curRequests)

	# start backend at 12:00
	timestamp = datetime.datetime.combine(datetime.date.today(), datetime.time(12, 0))
	runner.setBackendTime(processes, timestamp)

	curRequests = []
	curRequests.append(Request("http://www.battle.net", runner.UNKNOWN_USER, timestamp, '12345123123123', None))
	curRequests.append(Request("http://www.some-uncategorized-site.com", runner.ACCESS_OK, timestamp, opts.userId, None))

	allRequests.extend(curRequests)
	issueConcurrentRequests(opts, curRequests)

	# start backend at 13:00
	timestamp = datetime.datetime.combine(datetime.date.today(), datetime.time(13, 0))
	runner.setBackendTime(processes, timestamp)

	urls = map(str.strip, file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:NUM_REQUESTS])
	curRequests = []
	for url in urls:
		curRequests.append(Request(url, None, timestamp, opts.userId, None))

	allRequests.extend(curRequests)
	issueConcurrentRequests(opts, curRequests)

	# wait for backend to flush request log and check the log
	runner.stopBackend(processes)
	checkLogs(opts, allRequests)

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
