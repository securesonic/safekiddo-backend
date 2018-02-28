# Test scenario:
# 1. start webservice
# 2. start backend with local db and bcap cache disabled
# 3. send NUM_REQUESTS_PER_CONNECTION in NUM_CONNECTIONS in parallel (actually: asynchronously)
# 4. check average response time

import sys
import os
import time
import gevent
import traceback
from gevent import monkey

monkey.patch_all()

NUM_REQUESTS_PER_CONNECTION = int(sys.argv[1])
NUM_CONNECTIONS = int(sys.argv[2])

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def almostEqual(x, y, tolerance):
	med = (x + y) / 2
	dx = abs(x - med)
	dy = abs(y - med)
	allowedDiff = abs(tolerance * med) / 2
	return (dx < allowedDiff) and (dy < allowedDiff)

def issueRequests(opts, urls, idx):
	print 'connecting to %s:%s' % (opts.httpdHost, opts.httpdPort)
	
	connection = runner.createConnection(opts.httpdHost + ":" + opts.httpdPort)

	startTime = time.time()
	for url in urls:
		url = url.strip()
		#print 'requesting url:', url
		runner.sendRequest(connection, url, opts.userId)
		headers = runner.readResponse(connection)
		#print 'Result:', headers['result']
		#print 'duration: %.2f' % duration
	duration = time.time() - startTime
	print "total requests duration: %.2f" % duration
	return duration

def main():
	(opts, _) = optionParser.parseCommandLine()
	opts.timeout = 30000 # 30sec timeout

	#runner.startHttpd(opts, processes)
	#runner.startBackend(opts, processes)

	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:NUM_REQUESTS_PER_CONNECTION]

	jobs = [gevent.spawn(issueRequests, opts, urls, x) for x in range(NUM_CONNECTIONS)]
	gevent.joinall(jobs)
	jobResults = [job.value for job in jobs]
	
	avgDuration = reduce(lambda x, y: x + y, jobResults, 0.0) / NUM_CONNECTIONS
	expectedDuration = NUM_REQUESTS_PER_CONNECTION * 50.0 / 1000 # 50ms per request

	if opts.dbHost == "" or opts.dbHost == "localhost":
		print "db is local"
		expectedDuration = expectedDuration
	else:
		print "db is remote"
		expectedDuration = expectedDuration * 5
	print 'achieved %.2f req/s per connection; total: %.2f' % (NUM_REQUESTS_PER_CONNECTION / avgDuration, (NUM_REQUESTS_PER_CONNECTION / avgDuration) * NUM_CONNECTIONS)
	assert avgDuration < expectedDuration, "requests duration: %.2f, expected less than %.2f" % (avgDuration, expectedDuration)

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
