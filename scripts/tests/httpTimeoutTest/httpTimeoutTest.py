#!/usr/bin/python

import sys
import time
import os
from httplib import HTTPException

CLIENT_TIMEOUT=30

processes = {}

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def main():
	(opts, _) = optionParser.parseCommandLine()

	runner.startHttpd(opts, processes)
	connection = runner.createConnection('localhost:'+opts.httpdPort)
	
	time.sleep(CLIENT_TIMEOUT-6)
	# this should be ok, no timeout yet (possible timeout on backend/database) 
	runner.sendRequest(connection, 'onet.pl', opts.userId)
	runner.readResponse(connection, {}, True)
	
	time.sleep(CLIENT_TIMEOUT+3)
	# this should fail with connection closed (HttpException)
	runner.sendRequest(connection, 'onet.pl', opts.userId)
		
	try:
		runner.readResponse(connection)
		print "connection should be closed by server"
		sys.exit(-1)
	except HTTPException:
		pass
		
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
