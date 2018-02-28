#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
cd "$SCRIPT_DIR"

MGMT_FIFO=/sk/run/safekiddo/httpdMgmt.fifo

# remove old fifo just in case
rm -f MGMT_FIFO

(
# executed in a subshell
ulimit -c unlimited
cd /sk/bin/

./httpd.exe \
	--connectionLimit 800 \
	--logFilePath "/mnt/log/safekiddo/httpd.log" \
	--mgmtFifoPath "$MGMT_FIFO" \
	--admissionControlCustomSettingsFile "/sk/etc/admissionControlCustomSettings.accs" \
	2>> "/mnt/log/safekiddo/httpd.err"
)

# TODO: don't generate crash reports for httpd, first implement clean shutdown of httpd
#exitCode=$?
#
#if [[ $exitCode -ne 0 ]]; then
#	./createCrashReport.sh "httpd" "$exitCode"
#fi
