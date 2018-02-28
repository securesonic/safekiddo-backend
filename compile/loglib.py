import os
import sys
import logging

logger = logging.getLogger()
logger.setLevel(logging.NOTSET)
logHandler = logging.StreamHandler(sys.stdout)
logHandler.setFormatter(logging.Formatter("%(message)s", None))
logHandler.setLevel(logging.INFO)
logger.addHandler(logHandler)

logErr = logging.getLogger("err")
logErr.setLevel(logging.NOTSET)
logErr.propagate = False # otherwise its also logged to stdout (hopefully stderr is enough)
logHandler = logging.StreamHandler(sys.stderr)
logHandler.setFormatter(logging.Formatter("%(message)s", None))
logHandler.setLevel(logging.INFO)
logErr.addHandler(logHandler)

fileLog = logging.getLogger("fileLog")
fileLog.setLevel(logging.NOTSET)
fileLog.propagate = False # this is just for log message into file

def initFileLogger(fpath):
	fileHandler = logging.FileHandler(os.path.abspath(fpath), "w") # always new file...
	fmt = logging.Formatter('%(asctime)s %(levelname)-6s %(threadName)-6s %(message)s', None)
	fileHandler.setFormatter(fmt)
	fileHandler.setLevel(logging.DEBUG)
	logger.addHandler(fileHandler)
	logErr.addHandler(fileHandler)
	fileLog.addHandler(fileHandler)
	sys.stdout = FileLike(logger.info, sys.stdout)
	sys.stderr = FileLike(logErr.error, sys.stderr)
	logger.info("Log: %s" % os.path.abspath(fpath))

	fileLog.info("CALLED COMMAND: %s", str(sys.argv))
	for envname in "HOSTNAME USER PWD".split():
		fileLog.info("[ENV] %s=%s" % (envname, os.getenv(envname, None)))

class FileLike(object):
	def __init__(self, logFunc, origObj):
		self.logFunc = logFunc
		self.origObj = origObj

	def __getattr__(self, name):
		return getattr(self.origObj, name)

	def write(self, buff):
		strippedEnds = buff.rstrip()
		if strippedEnds:
			self.logFunc(strippedEnds)

info = logger.info
debug = logger.debug
