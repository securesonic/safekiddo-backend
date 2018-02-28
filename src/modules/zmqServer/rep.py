#!/usr/bin/python

import sys
import optparse
import reportGenerator

parser = optparse.OptionParser("usage: %prog [options] latencyRaport")
parser.add_option("-b", "--boxSize", dest="boxSize", default=10,
    type="int", help="box size in microseconds")
parser.add_option("-c", "--clientThreads", dest="clientThreads", default=1000,
    type="int", help="number of client threads")
parser.add_option("-w", "--workerThreads", dest="workerThreads", default=8,
    type="int", help="number of worker threads")
parser.add_option("-q", "--queries", dest="queries", default=1000,
    type="int", help="number of queries per client")
parser.add_option("-s", "--strategy", dest="strategy", default="",
    type="string", help="server strategy")
parser.add_option("-f", "--file", dest="file", default="",
    type="string", help="save to file instead of stdout")
(options, args) = parser.parse_args()

(program, bw, tatAvg, tat90, tat99) = reportGenerator.generate(sys.stdin, options)

if not options.file:
	print program
else:
	fn = options.file
	f = open(fn, "w+t")
	f.write(program)
	f.close()