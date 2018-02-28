#!/usr/bin/python

import sys
import time
import os

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

processes = {}

blockPageUrl = "https://dev-api.safekiddo.com/api/v1/block"

def printResponse(response):
	print "================================"
	print "c-icap response below:"
	print response
	print "================================"

def expectAccessOk(response):
	assert "Response was with status:204" in response, "Expected status 204 (Unmodified) from icap, response: %s" % (response,)
	printResponse(response)

def expectAccessDenied(response):
	assert "Response was with status:100" in response, "Expected status 100 (Continue) from icap, response: %s" % (response,)
	assert "REQMOD HEADERS:\n\tGET %s HTTP/1." % (blockPageUrl,) in response, \
		"Expected request modification to a block page, response: %s" % (response,)
	printResponse(response)

def expectUrlRewrite(response, targetUrl):
	assert "Response was with status:100" in response, "Expected status 100 (Continue) from icap, response: %s" % (response,)
	assert "REQMOD HEADERS:\n\tGET %s HTTP/1." % (targetUrl,) in response, \
		"Expected request rewrite to the following url: %s, response: %s" % (targetUrl, response)
	printResponse(response)

def main():
	(_, _) = optionParser.parseCommandLine()
	runner.startIcapServer(processes)

	for _ in range(10):
		response = runner.runIcapClient("onet.pl")
		expectAccessOk(response)

	for _ in range(10):
		response = runner.runIcapClient("narkotyki.pl")
		expectAccessDenied(response)

	response = runner.runIcapClient("http://google.pl/search?q=abc")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active")

	response = runner.runIcapClient("http://google.pl/search?q=abc&safe=off")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active")

	response = runner.runIcapClient("http://google.pl/search?safe=off&q=abc")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active")

	response = runner.runIcapClient("http://google.pl/search?s=42&safe=off&q=abc")
	expectUrlRewrite(response, "http://google.pl/search?s=42&q=abc&safe=active")

	# safe parameter without value
	response = runner.runIcapClient("http://google.pl/search?q=abc&safe=")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active")

	# safe parameter without even = sign
	response = runner.runIcapClient("http://google.pl/search?q=abc&safe")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active")

	# safe parameter with invalid value
	response = runner.runIcapClient("http://google.pl/search?q=abc&safe=42")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active")

	# double slash
	response = runner.runIcapClient("http://google.pl//search?q=abc")
	expectUrlRewrite(response, "http://google.pl//search?q=abc&safe=active")

	response = runner.runIcapClient("http://www.google.pl/search?q=abc")
	expectUrlRewrite(response, "http://www.google.pl/search?q=abc&safe=active")

	response = runner.runIcapClient("http://www.google.com.pl/search?q=abc")
	expectUrlRewrite(response, "http://www.google.com.pl/search?q=abc&safe=active")

	response = runner.runIcapClient("http://www.google.com/search?q=abc")
	expectUrlRewrite(response, "http://www.google.com/search?q=abc&safe=active")

	response = runner.runIcapClient("http://www.google.ie/search?q=abc")
	expectUrlRewrite(response, "http://www.google.ie/search?q=abc&safe=active")

	# just only "/search" in url is not enough
	response = runner.runIcapClient("http://www.example.com/search?q=abc")
	expectAccessOk(response)

	# just only "google" in url is not enough
	response = runner.runIcapClient("http://www.google.com")
	expectAccessOk(response)
	response = runner.runIcapClient("http://www.google.com/someaction?q=abc")
	expectAccessOk(response)

	# percent-decoding test
	response = runner.runIcapClient("http://google.pl/s%65arch?q=abc")
	expectUrlRewrite(response, "http://google.pl/s%65arch?q=abc&safe=active")
	response = runner.runIcapClient("http://google.pl/s%65arch?q=abc&saf%65=off")
	expectUrlRewrite(response, "http://google.pl/s%65arch?q=abc&safe=active")

	# verify fragment_id is ignored
	response = runner.runIcapClient("http://google.pl/search?q=abc#")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active#")
	response = runner.runIcapClient("http://google.pl/search?q=abc#top")
	expectUrlRewrite(response, "http://google.pl/search?q=abc&safe=active#top")
	response = runner.runIcapClient("http://google.pl/search?q=abc%23def") # %23 == '#'
	expectUrlRewrite(response, "http://google.pl/search?q=abc%23def&safe=active")

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
