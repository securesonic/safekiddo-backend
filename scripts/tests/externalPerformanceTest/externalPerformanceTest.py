#!/usr/bin/python

import sys
import time
import os
import multiprocessing
import optparse
import socket
import httplib
import signal

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner

def parseCommandLine():
	parser = optparse.OptionParser()
	parser.add_option('--steps', dest = 'steps', type = 'int', default = 10, action = 'store', help = 'number of steps in test')
	parser.add_option('--stepSize', dest = 'stepSize', type = 'int', default = 10, action = 'store', help = 'size (in clients) of a single step')
	parser.add_option('--urls', dest = 'urls', type = 'int', default = 100, action = 'store', help = 'number of urls to use in test')
	parser.add_option('--userId', dest = 'userId', type = 'string', default='70514', action = 'store', help = 'child uuid to use')
	parser.add_option('--httpdHost', dest = 'httpdHost', type = 'string', default = 'safekiddo-test-icap.cloudapp.net', action = 'store', help = 'webservice host')
	parser.add_option('--httpdPort', dest = 'httpdPort', type = 'string', default = '80', action = 'store', help = 'webservice port')

	(opts, args) = parser.parse_args()
	return opts, args


def issueRequestsCommon(host, urls, userId, shouldStopFunc):
	connection = runner.createConnection(host)
	startTime = time.time()

	numTimeouts = 0
	firstIteration = True
	numConsecutiveFailures = 0
	while shouldStopFunc is not None or firstIteration:
		if shouldStopFunc is not None and shouldStopFunc():
			break
		for url in urls:
			if shouldStopFunc is not None and shouldStopFunc():
				break
			url = url.strip()
			try:
				connectionOk = False
				runner.sendRequest(connection, url, userId)
				headers = runner.readResponse(connection, expectedHeaders = {}, timeoutExpected = None)
				if runner.isTimeout(headers):
					numTimeouts += 1
				assert headers['result'] != runner.UNKNOWN_USER, "please use existing userId as otherwise performance is meaningless"
				connectionOk = True
			except httplib.BadStatusLine:
				print "Warning: bad status line for: %s" % url
			except httplib.CannotSendRequest:
				print "Warning: cannot send request for: %s" % url
			if connectionOk:
				numConsecutiveFailures = 0
			else:
				numConsecutiveFailures += 1
				assert numConsecutiveFailures < 2, "too many consecutive failures, webservice is probably down"
				print "Warning: connection lost, reconnecting"
				connection.close()
				connection = runner.createConnection(host)
		firstIteration = False

	duration = time.time() - startTime
	return (duration, numTimeouts)

def issueTimedRequests(host, urls, userId):
	return issueRequestsCommon(host, urls, userId, shouldStopFunc = None)


class BackgroundProcess(multiprocessing.Process):
	def __init__(self, urls, host, userId, running):
		self.urls = urls
		self.host = host
		self.userId = userId
		self.running = running
		multiprocessing.Process.__init__(self)
		
	def run(self):
		issueRequestsCommon(self.host, self.urls, self.userId, shouldStopFunc = lambda: not self.running.value)

def main():
	(opts, _) = parseCommandLine()
	
	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()[:opts.urls]
	stepSize = opts.stepSize
	numberOfSteps = opts.steps
	
	hostAddr = socket.gethostbyname(opts.httpdHost)
	print "resolved %s to %s" % (opts.httpdHost, hostAddr)

	host = hostAddr + ":" + opts.httpdPort

	columnNames = ""
	latencyLine = ""
	performanceLine = ""
	timeoutsLine = ""

	step = 0

	def signalHandler(signum, frame):
		raise Exception("got signal %s" % (signum,))
	signal.signal(signal.SIGTERM, signalHandler)

	while True:
		(duration, numTimeouts) = issueTimedRequests(host, urls, opts.userId)
		numberOfThreads = len(threads) + 1
		performance = len(urls) * numberOfThreads / duration
		latency = duration / len(urls)
		timeoutsRatio = float(numTimeouts) / len(urls)
		print 'clients: %d, reqs/s: %.1f, lat: %.3f (duration %.2f, urls %d); timeouts ratio: %.2f' % (numberOfThreads,
				performance, latency, duration, len(urls), timeoutsRatio)
		columnNames += str(numberOfThreads)
		latencyLine += '%.3f' % latency
		performanceLine += '%.1f' % performance
		timeoutsLine += '%.2f' % timeoutsRatio
		
		step += 1
		if step > numberOfSteps:
			break
		
		columnNames += ','
		latencyLine += ','
		performanceLine += ','
		timeoutsLine += ','
	
		for _ in range(stepSize):
			thread = BackgroundProcess(urls, host, opts.userId, running)
			threads.append(thread)
			thread.start()
		
		time.sleep(5)

	columnNames += '\n'
	latencyLine += '\n'
	performanceLine += '\n'
	timeoutsLine += '\n'

	output = open("latency.txt", "w")
	output.write(columnNames)
	output.write(latencyLine)
	output.close()

	output = open("performance.txt", "w")
	output.write(columnNames)
	output.write(performanceLine)
	output.close()

	output = open("timeouts.txt", "w")
	output.write(columnNames)
	output.write(timeoutsLine)
	output.close()
			
	print "Finishing"

if __name__ == '__main__':
	try:
		cleanExit = True
		try:
			processes = {}
			threads = []
			running = multiprocessing.Value('b', True)
			main()
		except (SystemExit, KeyboardInterrupt):
			pass # --help in arguments
		except:
			cleanExit = False
			runner.testFailed(processes)
			raise
	finally:
		print "Stopping threads"

		running.value = False

		for t in threads:
			t.join()
			if cleanExit:
				assert t.exitcode == 0, "thread exitcode should be 0 but is %s" % t.exitcode
