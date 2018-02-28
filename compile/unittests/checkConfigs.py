"""
	This test just verify some flags
"""

import sys
import os

startdir = os.getcwd()

BASE_CONFIGS = [ "release", "debug", "gcov" ]
FLAG_GROUPS = [
		["opt", "noOpt"],
		["valgrind"],
		["sym"],
		["32", "64"],
]

def setupPythonPaths():
	buildScripts = os.path.join(startdir, os.path.dirname(__file__), "..", "..", "..", "build", "scripts")
	buildScripts = os.path.normpath(buildScripts)
	if not buildScripts in sys.path:
		sys.path.append(buildScripts)
	from Utils import pythonPathConfig
	print pythonPathConfig.initPythonPaths("hydraOST", "compileUTs")

def getAllConfigClasses():
	import hydraOSTConfig
	classes = []
	for attrname in dir(hydraOSTConfig):
		if not attrname.startswith("HydraOSTConfig") or attrname == "HydraOSTConfig":
			continue
		klass = getattr(hydraOSTConfig, attrname)
		if callable(klass):
			classes.append(klass)
	return classes

def getConfigNames():
	configsForTest = set()
	for baseConf in BASE_CONFIGS:
		for group in FLAG_GROUPS:
			for flag in group:
				configsForTest.add( "%s_%s" % (baseConf, flag))
	return configsForTest

def isConfigurationExcluded(constructor, config):
	if "_64" not in config and "Hpux" in constructor.__name__:
		print "Hpux 32-bit config is DISABLED [skip:%s]" % config
		return True
	return False

def main():
	setupPythonPaths()
	import wrpManager
	import wrplib
	for constructor in getAllConfigClasses():
		config = None
		confObj = None
		for config in getConfigNames():
			if isConfigurationExcluded(constructor, config):
				continue
			print "Creating %s with %s" % (constructor.__name__, config)
			wrpManager.resetEnv()
			confObj = constructor(config.split("_"))
		if confObj:
			wrplib.showConf(confObj.getString(), confObj.env)

if __name__ == "__main__":
	main()

