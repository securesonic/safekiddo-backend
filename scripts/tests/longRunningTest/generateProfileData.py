#!/usr/bin/python

import sys
import random

NUM_PROFILES = 10000
BASE_ID = 10 ** 9

def generateSql(templateFile):
	template = open(templateFile, 'rt').read()
	for offset in xrange(NUM_PROFILES):
		placeholders = {
			'childId' : BASE_ID + offset,
			'parentId' : BASE_ID + offset,
			'profileId' : BASE_ID + offset,
			'serial' : random.randint(10000000, 99999999),
			'uuid' : BASE_ID + offset,
			'birthDate' : '20%02d-%02d-%02d' % (random.randint(0,9), random.randint(1,9), random.randint(1,22))
		}
		sql = template
		for ph in placeholders.keys():
			sql = sql.replace('$%s' % ph, str(placeholders[ph]))
		print sql

if __name__ == '__main__':
	if len(sys.argv) == 2 and sys.argv[1] == '--remove':
		generateSql('dbDataTemplateRemove.sql')
	else:
		generateSql('dbDataTemplateInsert.sql')
