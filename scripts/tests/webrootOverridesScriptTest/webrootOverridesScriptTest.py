#!/usr/bin/python

import sys
import time
import os

VIOLENCE = "Violence"
ADULT_AND_PORNOGRAPHY = "Adult and pornography"
INTERNET_PORTALS = "Internet Portals"
processes = {}

TEXT_BOMB = """
brv9bWkXj9Rn9C3?~~~JY^xCx-t#FeYE)8*^Z^mRz?xE_hPj)W&)vVKYXZTKZVZE@5&P7~~~Y)#C%@h2!%C@#L7z##8mG++@J?3358PvhF2_)S*bh&3fNT2em~~~Rw5PXE5*FFn
Np$~~~Zkz5p=3@#4t@#s?FMF)vRehxZ8))kLTJ_~~~==V^wzNW8_&JWz)-n!Yc)rkzZ4y2f~~~%e)6X^s@=TWJteyvK@#3f)8sm-ELJSFL*h@#R@es@W@BsZ5h7msf5+tRTVvWESpE
b!h4v)W6E*?&)@#JW2r!yH5&?C)Zp!c*FVtTWzfXc9V~~~5@n=K8C%7PzkrVRb6fcs6Sn@#m5yE~~~!HZvE_n5w6M^26bT)$5*yWT&hNBT@#CZf@Lb8)E@#3VR-N!xcK9bh@#@#@6C
knnzRg73cPCke!W)#-~~~~~~CS)x6J?gsx~~~T~~~f)~~~Rc?8z2%~~~m_5P#G3^j)wK$gS$XMBS6Hr+G*=2)XZ)Xje)jHVFr-))c~~~4&My*E~~~e-Jvb))JjzRm)k3H%g6M5pkwsTr9*_N
"""

sys.path.append(os.path.abspath(__file__ + '/../../..'))
from library import runner, optionParser

def commandAddCategory(url, category):
	return "a\n%s\n%s\n" % (url, category)

def commandDeleteUrl(url):
	return "d\n%s\nmissclick\ny\n" % url

def commandPrintCategories():
	return "c\n"
	
def commandPrintOverrides():
	return "o\n"
	
def commandUpdateCategory(url, category):
	return "u\n%s\n%s\n" % (url, category)

def commandQuit():
	return "q"

def issueRequest(opts, url, expectedResult):
	print 'connecting to localhost:%s' % opts.httpdPort

	connection = runner.createConnection('localhost:' + opts.httpdPort)

	url = url.strip()
	print 'requesting url:', url
	runner.sendRequest(connection, url, opts.userId)
	headers = runner.readResponse(connection, expectedHeaders = {'result': expectedResult})
	print 'Result:', headers['result']
	print 'all headers:\n', headers


def main():
	(opts, _) = optionParser.parseCommandLine()

	runner.startHttpd(opts, processes)

	runner.startBackend(opts, processes)
	runner.setBackendTime(processes, "2014-09-12 10:05:59")

	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	# FirstScenario - "simple scenario test":
	# 1. Set category Violence to onet.pl [CATEGORY_BLOCKED_CUSTOM] and category Violence to bing.pl [CATEGORY_BLOCKED_CUSTOM]
	# 2. Update onet.pl category for Internet Portals [ACCESS_OK]
	# 3. Update onet.pl category for Adult and Pornography [CATEGORY_BLOCKED_CUSTOM]
	# 4. Delete onet.pl from webroot overrides [ACCESS_OK]
	# 5. Delete bing.pl from webroot overrides [ACCESS_OK]
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandAddCategory("http://www.onet.pl", VIOLENCE) +
		commandAddCategory("bing.pl", VIOLENCE) +
		commandQuit()
	)
	runner.stopWebrootOverridesScript(processes)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://bing.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandUpdateCategory("onet.pl", INTERNET_PORTALS) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://bing.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandUpdateCategory("onet.pl", ADULT_AND_PORNOGRAPHY) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://bing.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandDeleteUrl("onet.pl") +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://bing.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandDeleteUrl("bing.pl") +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	# SecondScenario - "lists test":
	# 1. Check categories list
	# 2. Check if overrides list is empty
	# 3. Set category Violence to onet.pl [CATEGORY_BLOCKED_CUSTOM]
	# 4. Set category Adult and Pornography to wp.pl [CATEGORY_BLOCKED_CUSTOM]
	# 5. Check if overrides list have correct values
	# 6. Delete wp.pl from webroot overrides
	# 7. Check if overrides have correct values
	# 8. Delete onet.pl from webroot overrides
	# 9. Checi if overrides have correct values
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandPrintCategories() +
		commandQuit()
	)
	assert "CATEGORIES LIST:\nAdult and pornography\nAbused drugs\nViolence\nIllegal\nInternet Portals\n" in response[0]
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandPrintOverrides() +
		commandQuit()
	)
	assert "APPLIED WEBROOT OVERRIDES LIST:\n0 Webroot Overrides Found\n" in response[0]
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandAddCategory("http://www.onet.pl", VIOLENCE) +
		commandAddCategory("wp.pl", ADULT_AND_PORNOGRAPHY) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.CATEGORY_BLOCKED_CUSTOM)

	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandPrintOverrides() +
		commandQuit()
	)
	runner.stopWebrootOverridesScript(processes)
	assert "APPLIED WEBROOT OVERRIDES LIST:\nonet.pl - Violence\nwp.pl - Adult and pornography\n" in response[0]
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandDeleteUrl("wp.pl") +
		commandPrintOverrides() +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	assert "APPLIED WEBROOT OVERRIDES LIST:\nonet.pl - Violence\n" in response[0]
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandDeleteUrl("onet.pl") +
		commandPrintOverrides() +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	assert "APPLIED WEBROOT OVERRIDES LIST:\n0 Webroot Overrides Found\n" in response[0]
	
	# ThirdScenario - "uncorrect values":
	# 1. Try to add incorrect urls to overrides table
	# 2. Try to set incorrect category for correct url
	# 3. Set category Violence to onet.pl [CATEGORY_BLOCKED_CUSTOM]
	# 4. Try to set category for onet.pl again
	# 5. Try to delete unexisting url
	# 6. Try to delete onet.pl, but reject in last step
	# 7. Try to update category of unexisting url 
	# 8. Try to update onet.pl category for unexisting one
	# 9. Update onet.pl category for Internet Portals [ACCESS_OK]
	# 10.Update onet.pl category for Violence [CATEGORY_BLOCKED_CUSTOM]
	# 11. Send textbomb to script
	# 12. Delete onet.pl from webroot overrides [ACCESS_OK]
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		"a\n%s\n%s\n%s\n%s\n%s\n%s\n" % ("asda//", "aaasd::s*&*/", "onet.pl", "ASDAdasd", "Categ#$#SDF", VIOLENCE) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	runner.sendCommandToWebrootOverridesScript(
		processes, 
		commandAddCategory("http://www.onet.pl", INTERNET_PORTALS) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		"d\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n" % ("bing.pl", "()()()(", "VVWWV.ASDASD.!@#", "onet.pl", "a", "v", "n", "y") +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		"u\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n" % ("bing.pl", "()()()(", "VVWWV.ASDASD.!@#", "onet.pl", "ASDAS", "Cat", "y", INTERNET_PORTALS) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		"u\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n" % ("bing.pl", "()()()(", "VVWWV.ASDASD.!@#", "onet.pl", "ASDAS", "Cat", "y", VIOLENCE) +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.CATEGORY_BLOCKED_CUSTOM)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
	runner.startWebrootOverridesScript(processes)
	response = runner.sendCommandToWebrootOverridesScript(
		processes, 
		"%s\nd\n%s\n%s\n" % (TEXT_BOMB, "onet.pl", "y") +
		commandQuit()
	)
	
	runner.reloadBackendOverrides(processes)
	issueRequest(opts, "http://www.onet.pl", runner.ACCESS_OK)
	issueRequest(opts, "http://www.bing.pl", runner.ACCESS_OK)
	issueRequest(opts, "www.wp.pl", runner.ACCESS_OK)
	
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
