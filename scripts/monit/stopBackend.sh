#!/usr/bin/python

import time
import os
import sys

sys.path.append(os.path.abspath(__file__ + '/../..'))
from library import mgmtLib

mgmtLib.issueMgmtCommand(None, 'backend', '/sk/run/safekiddo/', 'stop')
for i in range(0, 5):
	time.sleep(1)
	if not os.system('killall -0 backend.exe'):
		sys.exit(0)

# force a coredump
os.system('killall -SIGILL backend.exe')
