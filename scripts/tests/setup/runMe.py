#!/usr/bin/python

import os

# Don't use ' in commands (cause it is used when invoking runTest.sh)
# Commands may contain newlines.
# Note that python expands backslash sequences in the triple quote string (""" x """).
testCommand = {
	'WebrootDbUpdatesTest': 'cd bin/$CONFIG\n./webrootDbUpdatesTest.exe',
	'WebrootDbRemovalTest': 'cd bin/$CONFIG\n./webrootDbRemovalTest.exe',
	'WebrootCloudCacheTest': 'cd bin/$CONFIG\n./webrootCloudCacheTest.exe',
	'WebrootSAF108Test': 'cd bin/$CONFIG\n./webrootSAF108Test.exe',
	'LogSystemTest': 'cd bin/$CONFIG\n./logSystemTest.exe',
	'ReportsFunctionalTest': 'cd web/reports/reportsServer/tests/functionalTests/\n./runTests.py',
	'ReportsPerformanceTest': 'cd web/reports/reportsServer/tests/performanceTest/\n./performanceTest.py',
	'WP8MDMFunctionalTest': 'cd mdm/wp8/tests/functionalTest/\n./functionalTest.py',
	'WP8MDMExternalPerformanceTest': 'cd mdm/wp8/tests/externalPerformanceTest/\n./externalPerformanceTest.py'
}

testTemplateV1 = """\
DB_NAME=$BUILD_TAG

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f %(sqlFile)s

%(testScript)s --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}"

dropdb $DB_NAME
"""

testTemplateV2 = """\
DB_NAME=$BUILD_TAG

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql

%(testScript)s --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}"

dropdb $DB_NAME
"""

testTemplateNoDb = """\
%(testScript)s --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}"
"""

testTemplateWithLogsDb = """\
DB_NAME=$BUILD_TAG
DB_NAME_LOGS=${BUILD_TAG}_logs

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f %(sqlFile)s

sudo -u postgres createdb -O jenkins $DB_NAME_LOGS
psql --set ON_ERROR_STOP=1 -d $DB_NAME_LOGS -f sql/schema_logs.sql

%(testScript)s --dbName $DB_NAME --dbNameLogs $DB_NAME_LOGS "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}"

dropdb $DB_NAME
dropdb $DB_NAME_LOGS
"""

testCommand['ConnectionReestablishedTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/connectionReestablishedTest/connectionReestablishedTest.py'
}
testCommand['SimpleTimeoutTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/simpleTest/simpleTest.py'
}
testCommand['DatabaseReconnectTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/databaseReconnectTest/databaseReconnectTest.py'
}
testCommand['DayOfWeekTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_use_day_of_week.sql',
	'testScript': 'scripts/tests/useDayOfWeekTest/useDayOfWeekTest.py'
}
testCommand['HttpTimeoutTest'] = testTemplateNoDb % {
	'testScript': 'scripts/tests/httpTimeoutTest/httpTimeoutTest.py'
}
testCommand['TemporaryAccessTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/tmpAccessTest/tmpAccessTest.py'
}
testCommand['UnknownUserTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/unknownUserTest/unknownUserTest.py'
}
testCommand['FirstScenarioTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/firstTestScenario/firstTestScenario.py'
}
testCommand['SecondScenarioTest'] = testTemplateV2 % {
	'testScript': 'scripts/tests/secondTestScenario/secondTestScenario.py'
}
testCommand['ThirdScenarioTest'] = testTemplateV2 % {
	'testScript': 'scripts/tests/thirdTestScenario/thirdTestScenario.py'
}
testCommand['RequestLoggerFuncTest'] = testTemplateWithLogsDb % {
	'sqlFile': 'sql/test_scenario2_inserts.sql',
	'testScript': 'scripts/tests/requestLoggerTest/requestLoggerTest.py --timeout 20000'
}
testCommand['SslTest'] = testTemplateNoDb % {
	'testScript': 'scripts/tests/sslTest/sslTest.py'
}
testCommand['UrlCanonicalFormTest'] = testTemplateV2 % {
	'testScript': 'scripts/tests/urlCanonicalFormTest/urlCanonicalFormTest.py'
}
testCommand['UrlPatternsTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/urlPatternsTest/urlPatternsTest.py'
}
testCommand['WebrootUnavailableTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/webrootUnavailableTest/webrootUnavailableTest.py'
}
testCommand['AdmissionControlTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/admissionControlTest/admissionControlTest.py'
}
testCommand['WebrootOverridesTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/webrootOverridesTest/webrootOverridesTest.py'
}
testCommand['StatisticsTest'] = testTemplateV1 % {
	'sqlFile': 'sql/test_scenario1_inserts.sql',
	'testScript': 'scripts/tests/statisticsTest/statisticsTest.py'
}
testCommand['DatabaseCreationTest'] = """\
DB_NAME=$BUILD_TAG

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/test_scenario1_inserts.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/deletes.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/test_scenario2_inserts.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/deletes.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/test_scenario3_inserts.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/deletes.sql

dropdb $DB_NAME
"""

testCommand['SystemPerformanceTest'] = """\
DB_NAME=$BUILD_TAG

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/test_use_day_of_week.sql

scripts/tests/performanceTest/performanceTest.py --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}"
scripts/tests/performanceTest/performanceTest.py --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}" --ssl

dropdb $DB_NAME
"""

testCommand['SystemPerformanceTestNoLogsDb'] = """\
DB_NAME=$BUILD_TAG

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/test_use_day_of_week.sql

scripts/tests/performanceTest/performanceTest.py --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}" --dbNameLogs fake_db

dropdb $DB_NAME
"""

testCommand['SlowLogsDbPerfTest'] = """\
DB_NAME=$BUILD_TAG

sudo -u postgres createdb -O jenkins $DB_NAME
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/create_empty_profile_database.sql
psql --set ON_ERROR_STOP=1 -d $DB_NAME -f sql/test_use_day_of_week.sql

scripts/tests/slowLogsDbPerfTest/slowLogsDbPerfTest.py --dbName $DB_NAME "${SK_DB_OPTIONS[@]}" "${SK_WEBROOT_CREDS[@]}"

dropdb $DB_NAME
"""

testCommand['RequestLoggerPerfTest'] = """\
URL_FILE=$(readlink -f scripts/tests/alexa100k.urls.txt)

cd bin/$CONFIG
echo "testing 16 workers"
# subset of urls
/usr/bin/time ./requestLoggerTest.exe --dbUser "" --dbPassword "" --workers 16 -n 10000 --numIterations 2 --urlFile "$URL_FILE"
cd ../..
mv bin/$CONFIG/performance.txt performance_16.txt

cd bin/$CONFIG
echo "testing 1 worker"
# all available urls
/usr/bin/time ./requestLoggerTest.exe --dbUser "" --dbPassword "" --workers 1 -n 100000 --numIterations 5 --urlFile "$URL_FILE"
cd ../..
mv bin/$CONFIG/performance.txt performance_1.txt
"""

testCommand['LongRequestLoggerPerfTest'] = """\
URL_FILE=$(readlink -f scripts/tests/alexa100k.urls.txt)

cd bin/$CONFIG
echo "testing 16 workers"
# subset of urls
/usr/bin/time ./requestLoggerTest.exe --dbUser "" --dbPassword "" --workers 16 -n 10000 --numIterations 20 --urlFile "$URL_FILE"
cd ../..
mv bin/$CONFIG/performance.txt performance_16.txt

cd bin/$CONFIG
echo "testing 1 worker"
# all available urls
/usr/bin/time ./requestLoggerTest.exe --dbUser "" --dbPassword "" --workers 1 -n 100000 --numIterations 50 --urlFile "$URL_FILE"
cd ../..
mv bin/$CONFIG/performance.txt performance_1.txt
"""

testCommand['LongRequestLoggerPerfTestNoIndexes'] = """\
URL_FILE=$(readlink -f scripts/tests/alexa100k.urls.txt)

cd bin/$CONFIG
echo "testing 16 workers"
# subset of urls
/usr/bin/time ./requestLoggerTest.exe --dbNameLogs "safekiddo_logs_no_indexes" --dbUser "" --dbPassword "" --workers 16 -n 10000 --numIterations 20 --urlFile "$URL_FILE"
cd ../..
mv bin/$CONFIG/performance.txt performance_16.txt

cd bin/$CONFIG
echo "testing 1 worker"
# all available urls
/usr/bin/time ./requestLoggerTest.exe --dbNameLogs "safekiddo_logs_no_indexes" --dbUser "" --dbPassword "" --workers 1 -n 100000 --numIterations 50 --urlFile "$URL_FILE"
cd ../..
mv bin/$CONFIG/performance.txt performance_1.txt

psql -d "safekiddo_logs_no_indexes" -c "truncate table request_log"
"""

testCommand['FromTstWsFuncToTstWsLbPerformanceTest'] = """\
scripts/tests/externalPerformanceTest/externalPerformanceTest.py --httpdHost="TST-SK-WS-LB-565849150.eu-west-1.elb.amazonaws.com" --urls=100 --stepSize=20
"""

testCommand['SimpleIcapTest'] = """\
scripts/tests/simpleIcapTest/simpleIcapTest.py
"""

#######################################
# now build final command and print it

command = """
set -o errexit
# uncomment for debugging
#set -x
export CONFIG=$(cat ~/lastCompilationConfig)
export BUILD_ID=""" + os.getenv('BUILD_ID') + """
export BUILD_NUMBER=""" + os.getenv('BUILD_NUMBER') + """
export BUILD_TAG=""" + os.getenv('BUILD_TAG') + """
export JOB_NAME=""" + os.getenv('JOB_NAME') + """
# compile.sh installs in home directory
./scripts/tests/setup/runTest.sh '""" + testCommand[os.getenv('JOB_NAME')] + "'"

print command
