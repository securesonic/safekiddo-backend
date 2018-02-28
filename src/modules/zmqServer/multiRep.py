#!/usr/bin/python

import sys
import optparse
import reportGenerator
import subprocess
import os
import time

parser = optparse.OptionParser("usage: %prog [options]")
parser.add_option("-b", "--boxSize", dest="boxSize", default=10,
	type="int", help="box size in microseconds")
parser.add_option("-s", "--server", dest="server",
	type="string", help="path to server")
parser.add_option("-c", "--client", dest="client",
	type="string", help="path to client")
(options, args) = parser.parse_args()

workersSetup = [16]
clientsSetup = [1000]
pendingSetup = [1, 3, 10]
url = 'tcp://127.0.0.1:12345'

# workersSetup = workersSetup[0:2]
# clientsSetup = clientsSetup[0:2]
# pendingSetup = pendingSetup[0:2]

print 'Using server:', options.server
print 'Using client:', options.client

runId = time.strftime("fullreport_%Y%m%d_%H%M%S", time.localtime())
os.mkdir(runId)

results = {}

class Options:
	def __init__(self, boxSize, clientThreads, workerThreads, queriesPerClient, strategy, pending):
		self.boxSize = boxSize
		self.clientThreads = clientThreads
		self.workerThreads = workerThreads
		self.queriesPerClient = queriesPerClient
		self.strategy = strategy
		self.pending = pending

def generate3dPlot(legend, datFileName, runId):
	fscriptName = os.path.join(runId, 'tmp')
	scriptFile = open(fscriptName, 'w+t')
	scriptFile.write("""
set xlabel "workers"
set ylabel "pending"
set zlabel "%s"
set hidden3d
set dgrid3d 30,30
set terminal png size 1024,768
set output '%s/%s.png'
splot '%s/%s' using 1:2:3 with lines
""" % (legend, runId, datFileName, runId, datFileName))
	scriptFile.close()

	subprocess.call(['gnuplot', fscriptName])
	os.remove(fscriptName)

def generate3dScript(scriptName, legend, datFileName, runId):
	fscriptName = os.path.join(runId, scriptName)
	scriptFile = open(fscriptName, 'w+t')
	scriptFile.write("""
set xlabel "workers"
set ylabel "pending"
set zlabel "%s"
set hidden3d
set dgrid3d 30,30
splot '%s' using 1:2:3 with lines
pause -1
""" % (legend, datFileName))
	scriptFile.close()

numQueries = 10 ** 6
for numWorkers in workersSetup:
	for numClients in clientsSetup:
		queriesPerClient = numQueries / numClients
		for pending in pendingSetup:
			case = (numWorkers, numClients, pending)
			print 'testing case', case
			startTime = time.time()
			caseId = "multi_workers%d_clients%d_pending%d.txt" % (numWorkers, numClients, pending)
			reportFname = os.path.join(runId, caseId)
			server = subprocess.Popen([options.server, "-t", "%d" % numWorkers, "-a", url])
			clientCmd = "%s -t %d -a %s -q %d -p %d > %s" % (options.client, numClients, url, queriesPerClient, pending, reportFname)
			client = subprocess.Popen([clientCmd], shell = True)
			client.wait()
			subprocess.call(['kill', '-15', '%d' % server.pid])
			f = open(reportFname)
			opts = Options(options.boxSize, numClients, numWorkers, queriesPerClient, 'dealer', pending)
			(program, bw, tatAvg, tat90, tat99) = reportGenerator.generate(f, opts)
			print 'case tested; got results:', bw, tatAvg, tat90, tat99, "; running time=%.2f s" % (time.time() -startTime)
			imgFname = os.path.join(runId, caseId + ".agr")
			imgFile = open(imgFname, "w+t")
			imgFile.write(program)
			imgFile.close()
			results[case] = { 'bw': bw, 'tatAvg': tatAvg, 'tat90': tat90, 'tat99': tat99 }

descs = ['bw', 'tatAvg', 'tat90', 'tat99']
legend = { 'bw' : 'bandwidth [req/s]', 'tatAvg' : 'average TAT [ms]', 'tat90' : '90th percentile TAT [ms]', 'tat99' : '99th percentile TAT [ms]'}
for numClients in clientsSetup:
	reportBasename = os.path.join(runId, "clients_%d" % numClients)
	files = {}
	for desc in descs:
		files[desc] = open(reportBasename + '_' + desc + '.dat', "w+t")
# 	bwFile = open(reportBasename + "_bandwidth.dat", "w+t")
# 	tatAvgFile = open(reportBasename + "_tatAvg.dat", "w+t")
# 	tat90File = open(reportBasename + "_tat90.dat", "w+t")
# 	tat99File = open(reportBasename + "_tat99.dat", "w+t")
	
	for numWorkers in workersSetup:
		queriesPerClient = numQueries / numClients
		for pending in pendingSetup:
			case = (numWorkers, numClients, pending)
			result = results[case]
			for desc in descs:
				files[desc].write("%d %d %.2f\n" % (numWorkers, pending, result[desc]))

	for desc in descs:
		files[desc].close()
		prefix = ("clients_%d_" % numClients) + desc
		generate3dPlot(legend[desc], prefix + ".dat", runId)
		generate3dScript(prefix + '.p', legend[desc], prefix + ".dat", runId)
