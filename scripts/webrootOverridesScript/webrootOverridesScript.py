#!/usr/bin/python
import optparse
import psycopg2
import re

DB_HOST = "localhost"
HTTP = "http://"
HTTPS = "https://"
WWW = "www."

def normalizeUrl(url):
	if not url.islower():
		print "INFO: " + url + " WAS CHANGE TO LOWERCASE"
		url = url.lower()
	prefixes = [HTTP, HTTPS, WWW]
	for prefix in prefixes:
		if url.startswith(prefix):
			print "INFO: " + prefix + " REMOVED FROM URL"
			url = url[len(prefix):]
	return url	

def categoryFromInput():
	categoryId = None
	while categoryId is None:	
		listAllCategories(cursor)
		print ""
		
		categoryName = raw_input("webroot override category: ")
		
		cursor.execute(
			"""
			SELECT 
				ext_id
			FROM 
				categories
			WHERE
				name = %s
			""",
			(categoryName,)
		)
		
		result = cursor.fetchone()
		
		if result is None:
			printErrorTryAgain("category %s does not exist in database" % categoryName)
		else:
			categoryId = result[0]
		print ""
	return (categoryId, categoryName)
	
def urlFromInput():
	url = None
	while url is None:
		urlToCheck = raw_input("URL of new Webroot Override: ")
		urlToCheck = normalizeUrl(urlToCheck)
		
		#REGEX from DJANGO URLValidator
		regex = re.compile(
			r'^(?:(?:[A-Z0-9](?:[A-Z0-9-]{0,61}[A-Z0-9])?\.)+(?:[A-Z]{2,6}\.?|[A-Z0-9-]{2,}(?<!-)\.?)|' # domain...
			r'localhost|' # localhost...
			r'\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}|' # ...or ipv4
			r'\[?[A-F0-9]*:[A-F0-9:]+\]?)' # ...or ipv6
			r'(?::\d+)?' # optional port
			r'(?:/?|[/?]\S+)$', re.IGNORECASE
		)
		
		if regex.match(urlToCheck):
			cursor.execute(
				"""
				SELECT 
					count(*)
				FROM 
					webroot_overrides 
				WHERE
					address = %s
				""",
				(urlToCheck,)
			)
			
			if cursor.fetchone()[0] == 0:
				#Everything is OK with URL
				url = urlToCheck
			else:
				printErrorTryAgain("URL '%s' exists in database" % urlToCheck)
		else:
			printErrorTryAgain("URL is in invalid format")
		print ""
	return url

def getUrlThatExistsInDatabaseWithItsCategory(inputMsg):
	urlThatExistsInDatabase = None
	urlCategory = None
	while urlThatExistsInDatabase is None:
		listAllWebrootOverrides(cursor)
		urlToCheck = raw_input(inputMsg)
		
		cursor.execute(
			"""
			SELECT 
				name
			FROM 
				webroot_overrides 
			LEFT JOIN
				categories
			ON
				code_categories_ext_id = ext_id 
			WHERE
				address = %s
			""",
			(urlToCheck,)
		)
		
		result = cursor.fetchone()
		if result is not None:
			urlThatExistsInDatabase = urlToCheck
			urlCategory = result[0]
		else:
			printErrorTryAgain("URL '%s' does not exist in database" % urlToCheck)
	return (urlThatExistsInDatabase, urlCategory)

def addWebrootOverride(connection, cursor):	
	url = urlFromInput()
	(categoryId, categoryName) = categoryFromInput()
		
	cursor.execute(
		"""
		INSERT INTO 
			webroot_overrides
		(
			address,
			code_categories_ext_id
		)
		VALUES
		(
			%s,
			%s
		)
		""",
		(url,categoryId)
	)
	
	connection.commit()
	print "%s ADDED SUCCESSFULLY WITH CATEGORY %s" % (url, categoryName)

def connectToDatabase(options):
	return psycopg2.connect(
		database = options.dbName,
		user = options.dbUser,
		host = options.dbHost,
		port = options.dbPort
	)
	
def printErrorTryAgain(msg):
	print "ERROR: %s, TRY AGAIN \n" % msg
	
def printMenu():
	print ""
	print "AVAIBLE COMMANDS:"
	print ""
	print "Managment:"
	print "a - add new webroot override"
	print "d - delete selected url from database"
	print "u - update category in webroot override"
	print ""
	print "Lists:"
	print "c - print list of all avaible webroot categories"
	print "o - print list of all applied webroot overrides"
	print ""
	print "Others:"
	print "q - quits script"
	print ""

def deleteWebrootOverride(connection, cursor):
	(urlThatExistsInDatabase, urlCategory) = getUrlThatExistsInDatabaseWithItsCategory("URL to delete: ")
	while urlThatExistsInDatabase is None:
		listAllWebrootOverrides(cursor)
		urlToCheck = raw_input("URL to delete: ")
		
		cursor.execute(
			"""
			SELECT 
				name
			FROM 
				webroot_overrides 
			LEFT JOIN
				categories
			ON
				code_categories_ext_id = ext_id 
			WHERE
				address = %s
			""",
			(urlToCheck,)
		)
		
		result = cursor.fetchone()
		if result is not None:
			urlThatExistsInDatabase = urlToCheck
			urlCategory = result[0]
		else:
			printErrorTryAgain("this URL does not exist in database")
		
	print "ARE YOU SURE TO DELETE %s WITH CATEGORIZATION %s FROM DATABASE? (y/n)" % (urlThatExistsInDatabase, urlCategory)
	
	correctDecision = False
	while not correctDecision:
		decision = raw_input("decision: ")
		print ""
		
		if decision == "y":
			correctDecision = True
			
			cursor.execute(
				"""
				DELETE FROM
					webroot_overrides 
				WHERE
					address = %s
				""",
				(urlThatExistsInDatabase,)
			)
			
			connection.commit()
			print "%s DELETED" % urlThatExistsInDatabase
		
		elif decision == "n":
			correctDecision = True
			print "COMMAND CANCELED"
		
		else:
			printErrorTryAgain("decision is not recognized, type 'y' or 'n'")
	
def listAllWebrootOverrides(cursor):
	cursor.execute(
		"""
		SELECT 
			address,
			name
		FROM 
			webroot_overrides 
		LEFT JOIN
			categories
		ON
			code_categories_ext_id = ext_id 
		ORDER BY 
			address
		ASC
		"""
	)
	
	print "APPLIED WEBROOT OVERRIDES LIST:"
	overridesNumber = 0
	for queryResult in cursor:
		overridesNumber += 1
		print '%s - %s' % ( queryResult[0], queryResult[1])
	
	if overridesNumber == 0:
		print "0 Webroot Overrides Found"
		
def listAllCategories(cursor):
	cursor.execute(
		"""
		SELECT
			name
		FROM
			categories
		"""
	)
	
	print "CATEGORIES LIST:"
	for queryResult in cursor:
		print '%s' % queryResult[0]
	
def updateWebrootOverride(connection, cursor):
	(urlThatExistsInDatabase, urlCategory) = getUrlThatExistsInDatabaseWithItsCategory("URL to update: ")
	(categoryId, categoryName) = categoryFromInput()
	
	cursor.execute(
		"""
		UPDATE
			webroot_overrides
		SET
			(
				code_categories_ext_id
			)
		=
			(
				%s
			)
		WHERE
			address = %s
		""",
		(categoryId, urlThatExistsInDatabase)
	)
	
	connection.commit()
	print "%s CATEGORY UPDATED FROM %s TO %s" % (urlThatExistsInDatabase, urlCategory, categoryName)
		
parser = optparse.OptionParser()
parser.add_option('--dbHost', dest = 'dbHost', type = 'string', default = DB_HOST, action = 'store', help = 'host with profile db')
parser.add_option('--dbPort', dest = 'dbPort', type = 'string', default = 5432, action = 'store', help = 'db port')
parser.add_option('--dbName', dest = 'dbName', type = 'string', default = 'safekiddo', action = 'store', help = 'database name')
parser.add_option('--dbUser', dest = 'dbUser', type = 'string', action = 'store', help = 'database username')
(options, args) = parser.parse_args()

connection = connectToDatabase(options)
cursor = connection.cursor()

while True:
	printMenu()
	
	command = raw_input("your command: ")
	
	print ""
	if command == "a":
		addWebrootOverride(connection, cursor)
	elif command == "d":
		deleteWebrootOverride(connection, cursor)	
	elif command == "u":
		updateWebrootOverride(connection, cursor)
	elif command == "c":
		listAllCategories(cursor)
	elif command == "o":
		listAllWebrootOverrides(cursor)
	elif command == "q":
		break
	else:
		print "Command not found \n"

connection.close()
