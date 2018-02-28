#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
cd "$SCRIPT_DIR"

source configuration.sh

if [[ -z $NODE_ID ]]; then
	echo "NODE_ID is not set (backend not configured)"
	exit 1
fi >&2

MGMT_FIFO=/sk/run/safekiddo/backendMgmt.fifo

# remove old fifo just in case
rm -f "$MGMT_FIFO"

(
# executed in a subshell
ulimit -c unlimited

# TODO: paths should be configured in single separate file and sourced
cd /sk/bin/

export PGPASSFILE=/sk/bin/pgpass

./backend.exe \
	--address "tcp://*:7777" \
	--dbHost "$CONFIG_DB_HOST" \
	--dbName "skcfg" \
	--dbHostLogs "$LOG_DB_HOST" \
	--dbNameLogs "sklog" \
	--dbUser "backend" \
	--dbPassword "" \
	--webrootOem "ardura" \
	--webrootDevice "prod_ardura_1" \
	--webrootUid "ardura_dmz1223p$NODE_ID" \
	--logFilePath "/mnt/log/safekiddo/backend.log" \
	--mgmtFifoPath "$MGMT_FIFO" \
	--statisticsFilePath "/mnt/log/safekiddo/backend.stats" \
	2>> "/mnt/log/safekiddo/backend.err"
)

exitCode=$?

if [[ $exitCode -ne 0 ]]; then
	./createCrashReport.sh "backend" "$exitCode"
fi

