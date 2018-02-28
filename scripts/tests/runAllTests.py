#!/usr/bin/python

import sys
import os
import stat
import subprocess

repoDir = os.path.abspath(os.path.join(__file__, '../../../'))

print repoDir

executable = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH

def runTest(filename):
	if not filename.endswith('runAllTests.py'):
		print 'Running test:', filename
		subprocess.call(filename)
		print 'Test finished'
		print

# run python tests
for root, dir, files in os.walk(repoDir + '/scripts/tests'):
	for f in files:
		filename = os.path.join(root, f)
		if os.path.isfile(filename):
			st = os.stat(filename)
			mode = st.st_mode
			if mode & executable:
				runTest(filename)
				
for root, dir, files in os.walk(repoDir + '/bin/'):
	for f in files:
		if f.endswith("Test.exe"):
			filename = os.path.join(root, f)
			if os.path.isfile(filename):
				st = os.stat(filename)
				mode = st.st_mode
				if mode & executable:
					runTest(filename)