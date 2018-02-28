# Test scenario:
# 1. send request every INTERVAL_MS in NUM_CONNECTIONS in parallel (actually: asynchronously)
# 2. print statistics every 1 minute

import signal
import sys
import os
import time
import gevent
import random
import traceback
import generateProfileData
from gevent import monkey

monkey.patch_all()

NUM_CONNECTIONS = int(sys.argv[1])
INTERVAL_MS = int(sys.argv[2])
STAT_THRESHOLD = 600 # ten minutes

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

import httplib
import socket
import ssl

def connect_patched(self):
	"Connect to a host on a given (SSL) port."
	
	sock = socket.create_connection((self.host, self.port), self.timeout)
	if self._tunnel_host:
		self.sock = sock
		self._tunnel()
	self.sock = ssl.wrap_socket(sock, self.key_file, self.cert_file, ssl_version=ssl.PROTOCOL_SSLv3)

httplib.HTTPSConnection.connect = connect_patched


def createConnection(opts):
	return runner.createConnection(opts.httpdHost + ":" + opts.httpdPort, isHttps = True)

tats = []
sumTat = 0
numRequests = 0
delay = 0

def issueRequests(opts, urls, idx):
	global tats
	global sumTat
	global numRequests
	global delay 
	
	print idx,'connecting to %s:%s' % (opts.httpdHost, opts.httpdPort)
	
	connection = createConnection(opts)

	lastTick = time.time()
	urlIdx = random.randint(1, 1000) # each client starts from random url
	while True:
		if urlIdx == len(urls):
			urlIdx = 0
		nextTick = lastTick + INTERVAL_MS / 1000.0
		
		if time.time() > nextTick:
			delay = time.time() - nextTick
		while time.time() < nextTick:
			time.sleep(INTERVAL_MS / 1000.0 / 10) # sleep 1/10th of INTERVAL_MS
		
		currentTick = nextTick
		
		url = urls[urlIdx]
		url = url.strip()
		userId = random.randint(generateProfileData.BASE_ID, generateProfileData.BASE_ID + generateProfileData.NUM_PROFILES)
		#userId = 70514
		#print 'requesting url:', url
		currentTime = time.time()
		runner.sendRequest(connection, url, userId)
		try:
			headers = runner.readResponse(connection, timeoutExpected = None)
		except:
			print 'Got exception reading response; connection closed?'
			connection.close()
			connection = createConnection(opts)
			headers = {'result': 'FAILED'}
			
		duration = time.time() - currentTime
		if headers['result'] != '0' and False:
			print currentTime, 'BLOCKED!', url, headers
		tats.append(duration)
		sumTat += duration
		numRequests += 1
		urlIdx += 1
		lastTick = currentTick
		
	print "total requests duration: %.2f" % duration
	return duration

printStatsFlag = False
def signalHandler(signum, frame):
	global printStatsFlag
	printStatsFlag = True

def printStats():
	global tats
	global sumTat
	global numRequests
	global printStatsFlag
	global delay

	expectedBW = 1000.0 / INTERVAL_MS * NUM_CONNECTIONS
	lastPrintTime = time.time()
	while True:
		time.sleep(1)
		if printStatsFlag and numRequests > 0:
			currentTime = time.time()
			tats.sort()
			bw = (0.0 + numRequests) / (currentTime - lastPrintTime)
			tatAvg = 1000.0 * sumTat / numRequests
			tat90 = (1000 * tats[int(numRequests * 0.9)])
			tat99 = (1000 * tats[int(numRequests * 0.99)])
			print 'Elapsed since last print: %.2f' % (currentTime - lastPrintTime)
			print 'NumRequests:', numRequests
			print 'BW: %.2f req/s (expected %.2f req/s)' % (bw, expectedBW)
			print 'Avg TAT:  %.2f ms' % tatAvg
			print '90th TAT: %.2f ms' % tat90
			print '99th TAT: %.2f ms' % tat99
			print 'delay: %.2f s' % delay
			statsFile = open('lrtStats.txt', 'w+t')
			statsFile.write('%.2f,%d,%.2f,%.2f,%.2f,%.2f,%.2f\n' % (currentTime, numRequests, bw, expectedBW, tatAvg, tat90, tat99))
			statsFile.close()
			tats = []
			sumTat = 0
			numRequests = 0
			lastPrintTime = currentTime
			printStatsFlag = False

def main():
	(opts, _) = optionParser.parseCommandLine()
	opts.timeout = 30000 # 30sec timeout

	urls = file(os.path.abspath(__file__ + '/../../alexa100k.urls.txt')).readlines()

	signal.signal(signal.SIGUSR1, signalHandler)
	jobs = [gevent.spawn(issueRequests, opts, urls, x) for x in range(NUM_CONNECTIONS)]
	jobs.append(gevent.spawn(printStats))
	gevent.joinall(jobs)
	
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
