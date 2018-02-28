#!/usr/bin/python

# this is a program for painless interactive safekiddo backend configuration

import sys
import os
import dialog
import re
import socket

# simple dialog boxes width
BOX_WIDTH = 50

# is slightly sucks this is hardcoded here and in makefile; but only slightly
MONIT_SCRIPTS_DIR = os.getenv('MONIT_SCRIPTS_DIR', '/sk/scripts/monit/')
CONFIG_FILE_NAME = os.getenv('CONFIG_FILE_NAME', 'configuration.sh')

# template for autoconfiguration of backend
CONFIG_FILE_TEMPLATE = """
# NODE_ID is used for webroot key determination
NODE_ID=%(NODE_ID)s

# host with skcfg db
CONFIG_DB_HOST=%(CONFIG_DB_HOST)s

# host with log db
LOG_DB_HOST=%(LOG_DB_HOST)s
"""

# options in main menu
VIEW = "1)", "View current configuration"
AUTO_DEMO = "2)", "Autoconfigure for demo environment"
AUTO_PROD = "3)", "Autoconfigure for production environment"
MANUAL_EDIT ="4)", "Manual config"
QUIT = "5)", "Quit"

NODE_ID_INPUT_DESCRIPTION = \
	'NODE_ID is used to determine webroot\n' + \
	'key used on this instance.\n' + \
	'Valid ids are from range 01 to 99.\n' + \
	'Please use 01 and up for production instances\n' + \
	'(like: 04 for sk-ws-04)\n' + \
	'and 90 and down for demo instances.\n\n' + \
	'Enter NODE_ID:'

# deployment environment
class Environment:
	def __init__(self, dbConfig, knownNodes):
		self._dbConfig = dbConfig
		self._knownNodes = knownNodes 

	def getDbConfig(self):
		return self._dbConfig

	def getDefaultNodeIdForThisHost(self):
		myName = socket.getfqdn()
		for (nodeName, nodeId) in self._knownNodes.items():
			if nodeName == myName:
				return nodeId
		return None
			
# FIXME:
# instances and db config should not be hardcoded; instead it should be based on amazon ec2/rds cli:
# http://docs.aws.amazon.com/AWSEC2/latest/CommandLineReference/Welcome.html
# http://docs.aws.amazon.com/AmazonRDS/latest/CommandLineReference/Welcome.html

DEMO_ENVIRONMENT = Environment(
	dbConfig = {
		"CONFIG_DB_HOST": "tst-skcfg.ccyc3196jini.eu-west-1.rds.amazonaws.com",
		"LOG_DB_HOST": "tst-sklog.ccyc3196jini.eu-west-1.rds.amazonaws.com",
	},
	knownNodes = {
		"ip-10-0-16-217.eu-west-1.compute.internal": "96"
	}
)

PROD_ENVIRONMENT = Environment(
	dbConfig = {
		"CONFIG_DB_HOST": "skcfg.ccyc3196jini.eu-west-1.rds.amazonaws.com",
		"LOG_DB_HOST": "sklog.ccyc3196jini.eu-west-1.rds.amazonaws.com",
	},
	knownNodes = {
		"ip-10-0-4-35.eu-west-1.compute.internal": "01",
		"ip-10-0-29-237.eu-west-1.compute.internal": "02",
	}
)

def getConfigPath():
	return os.path.join(MONIT_SCRIPTS_DIR, CONFIG_FILE_NAME)
					
def loadConfig():
	config = {}
	configData = file(getConfigPath(), "rt").readlines()
	for configLine in configData:
		configLine = configLine.strip()
		# ignore comments
		if configLine.startswith('#') or configLine.find("=") == -1:
			continue
		(paramName, paramValue) = configLine.split("=")
		config[paramName.strip()] = paramValue.strip()
	return config

def isConfigured(config):
	configured = True
	for paramValue in config.values():
		configured = configured and len(paramValue) > 0
	return configured

def mainMenu(d, configured):
	configuredMsg = ""
	if not configured:
		configuredMsg = "\Zb\Z1*NOT*\Zn "
	(code, tag) = d.menu(
		"Backend is %sconfigured" % (configuredMsg, ),
		width = 70,
		height = 15,
		choices = [
			VIEW,
			AUTO_DEMO,
			AUTO_PROD,
			MANUAL_EDIT,
			QUIT,
		]
	)
	return code, tag

def viewConfig(d):
	d.textbox(getConfigPath(), width = 76)

def isNodeIdValid(nodeId):
	return re.match('\d\d', nodeId) is not None

def autoConfig(d, environment):
	nodeId = environment.getDefaultNodeIdForThisHost()
	while nodeId is None:
		(code, nodeId) = d.inputbox(NODE_ID_INPUT_DESCRIPTION, width = BOX_WIDTH, height = 15)
		nodeId = nodeId.strip()
		if code == d.DIALOG_OK:
			if not isNodeIdValid(nodeId):
				d.msgbox("Entered node id is invalid; please try again")
				nodeId = None
				continue
		else:
			# user cancelled operation
			return
			
	configData = file(getConfigPath(), "w+t")
	config = environment.getDbConfig().copy()
	config['NODE_ID'] = nodeId
	configData.write(CONFIG_FILE_TEMPLATE % config)
		
	d.msgbox(
		"New configuration written to file:\n%s" % getConfigPath(),
		width = BOX_WIDTH
	)
	
def manualEdit(d):
	# now this is a bit hackish, because ubuntu's python-dialog is outdated
	(code, newConfig) = d._perform(
		["--editbox", getConfigPath(), "30", "76"]
	)
	if code == d.DIALOG_OK:
		configData = file(getConfigPath(), "w+t")
		configData.write(newConfig)
		
		d.msgbox(
			"New configuration written to file:\n%s" % getConfigPath(),
			width = BOX_WIDTH
		)

def reallyQuit(d):
	code = d.yesno(
		"Backend is \Zb\Z1*NOT*\Zn configured!\nWould you like to continue configuring?",
		width = BOX_WIDTH
	)
	return code == d.DIALOG_CANCEL # option No selected

def configureBackend(forceReconfigure = False):
	currentConfig = loadConfig()

	# do not configure already configured instance
	if isConfigured(currentConfig) and not forceReconfigure:
		print 'Backend already configured; use --force option if you want to reconfigure it anyway'
		sys.exit(0) 
	
	d = dialog.Dialog()
	d.add_persistent_args(["--backtitle", "Safekiddo backend configuration"])
	d.add_persistent_args(["--colors"])
	
	while True:
		fastQuit = isConfigured(currentConfig) # do not ask user if really wants to quit if backend is configured 
		(code, tag) = mainMenu(d, isConfigured(currentConfig))
		if code != d.DIALOG_OK:
			# attempt to quit?
			if fastQuit or reallyQuit(d):
				break
		else:
			# some option selected
			if tag == VIEW[0]:
				viewConfig(d)
			elif tag == AUTO_DEMO[0]:
				autoConfig(d, DEMO_ENVIRONMENT)
			elif tag == AUTO_PROD[0]:
				autoConfig(d, PROD_ENVIRONMENT)
			elif tag == MANUAL_EDIT[0]:
				manualEdit(d)
			elif tag == QUIT[0]:
				if fastQuit or reallyQuit(d):
					break
		currentConfig = loadConfig()
	
if __name__ == '__main__':
	# lousy cmdline support
	configureBackend(len(sys.argv) > 1 and sys.argv[1] == '--force')
	