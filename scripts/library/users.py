import getpass
import os

USERS = {
	'sachanowicz'	: 0,
	'mmarzec'		: 1,
	'zytka'			: 2,
	'jenkins'		: 3,
	'9ld'			: 4,
	'jackowski'		: 5,
}

# this ports are used by kernel for outgoing connections: 32768..61000
BASE_PORT = 20000

def getUserPort():
	return userPort

def computeUserPort():
	currentUser = getpass.getuser()
	pid = os.getpid()
	return BASE_PORT + (pid % 100) * 100 + USERS[currentUser] * 10

# in case we want to make it random
userPort = computeUserPort()
