#!/usr/bin/python

import zmq
import time
import struct
import sys
import os
from optparse import OptionParser

from proto import safekiddo_pb2 as sk

def reply(msg, options):
	request = sk.Request()
	request.ParseFromString(msg)
	if options.wait > 0:
		time.sleep(options.wait)
	resp = sk.Response()
	resp.result = sk.Response.RES_OK
	assert resp.IsInitialized()
	return resp.SerializeToString()

def parseCommandLine():
	parser = OptionParser()

	parser.add_option("-p", "--port", type="int", default=7777, help="ZMQ port number [%default]")
	parser.add_option("-w", "--wait", type="int", default=0, help="Wait seconds before response [%default]")

	(options, _) = parser.parse_args()
	return options 

if __name__ == "__main__":
	options = parseCommandLine()
	
	context = zmq.Context()
	socket = context.socket(zmq.REP)
	socket.bind("tcp://*:%d" % options.port)

	print "Server waiting for clients"
	
	while True:
		msg = reply(socket.recv(), options)
		socket.send(msg)

	print "Finishing"
