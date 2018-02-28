import optparse
import users

BACKEND_PORT = 0
HTTPD_PORT = 1
TUNNEL_PORT = 2 # for tests that need to use tunnel

DB_HOST = 'localhost'

def parseCommandLine():
	userPort = users.getUserPort()
	
	parser = optparse.OptionParser()
	parser.add_option('--workers', dest = 'workers', type = 'int', default = 16, action = 'store', help = 'number of worker threads in backend')
	parser.add_option('--backendTransport', dest = 'backendTransport', type = 'string', default = 'tcp://', action = 'store', help = 'backend transport')
	parser.add_option('--backendPort', dest = 'backendPort', type = 'string', default = '%d' % (userPort + BACKEND_PORT,), action = 'store', help = 'backend port')
	parser.add_option('--dbHost', dest = 'dbHost', type = 'string', default = DB_HOST, action = 'store', help = 'host with profile db')
	parser.add_option('--dbPort', dest = 'dbPort', type = 'string', default = 5432, action = 'store', help = 'db port')
	parser.add_option('--dbHostLogs', dest = 'dbHostLogs', type = 'string', default = None, action = 'store', help = 'host with logs db')
	parser.add_option('--dbPortLogs', dest = 'dbPortLogs', type = 'string', default = None, action = 'store', help = 'logs db port')
	parser.add_option('--dbName', dest = 'dbName', type = 'string', default = 'skcfg', action = 'store', help = 'database name')
	parser.add_option('--dbNameLogs', dest = 'dbNameLogs', type = 'string', default = 'sklog', action = 'store', help = 'logs database name')
	parser.add_option('--dbUser', dest = 'dbUser', type = 'string', default = 'backend', action = 'store', help = 'database username')
	parser.add_option('--dbPassword', dest = 'dbPassword', type = 'string', default = 'password', action = 'store', help = 'database password')
	parser.add_option('--userId', dest = 'userId', type = 'string', default='70514', action = 'store', help = 'user id')
	parser.add_option('--httpdPort', dest = 'httpdPort', type = 'string', default = '%d' % (userPort + HTTPD_PORT,), action = 'store', help = 'webservice port')
	parser.add_option('--httpdHost', dest = 'httpdHost', type = 'string', default = 'localhost', action = 'store', help = 'host with httpd')
	parser.add_option('--timeout', dest = 'timeout', type = 'string', default = '10000', action = 'store', help = 'backend timeout [ms]')
	parser.add_option('--clear', dest = 'clear', default = False, action = 'store_true', help = 'force clear all current user instances')
	parser.add_option('--ssl', dest = 'ssl', default = False, action = 'store_true', help = 'use SSL for http connections')
	parser.add_option('--tunnelPort', dest = 'tunnelPort', type = 'string', default = '%d' % (userPort + TUNNEL_PORT,), action = 'store',
		help = 'tunnel port (if needed by test)')
	parser.add_option('--webrootOem', dest = 'webrootOem', type = 'string', default = None, action = 'store', help = 'webroot oem')
	parser.add_option('--webrootDevice', dest = 'webrootDevice', type = 'string', default = None, action = 'store', help = 'webroot device')
	parser.add_option('--webrootUid', dest = 'webrootUid', type = 'string', default = None, action = 'store', help = 'webroot uid')
	parser.add_option('--webrootConfigFilePath', dest = 'webrootConfigFilePath', type = 'string', default = None, action = 'store', help = 'webroot config file path')
	parser.add_option('--logLevel', dest = 'logLevel', type = 'string', default = 'DEBUG1', action = 'store', help = 'log level')
	parser.add_option('--statisticsFilePath', dest = 'statisticsFilePath', type = 'string', default = 'backend.stats', action = 'store',
		help = 'statistics file path')
	parser.add_option('--statisticsPrintingInterval', dest = 'statisticsPrintingInterval', type = 'string', default = 3, action = 'store',
		help = 'statistics printing interval in seconds')
	parser.add_option('--admissionControlCustomSettingsFile', dest = 'admissionControlCustomSettingsFile', type = 'string',
		default = 'admissionControlCustomSettings.accs', action = 'store', help = 'admission control custom settings file path')
	parser.add_option('--useForwardedFor', dest = 'useForwardedFor', default = False, action = 'store_true',
		help = 'use X-Forwarded-For http header')


	(opts, args) = parser.parse_args()
	return opts, args
