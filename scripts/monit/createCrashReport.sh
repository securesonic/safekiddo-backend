#!/bin/bash

umask 022 # SAF-527

usage()
{
	echo "Usage: $(basename "$0") <backend|httpd> exit_code"
	echo
	echo "Makes a copy of logs of selected program. Adds information about exit code."
} >&2

if [[ $# -ne 2 ]]; then
	usage
	exit 2
fi

program=$1
exit_code=$2

if [[ $program != "backend" && $program != "httpd" ]]; then
	echo "Unknown program $program" >&2
	usage
	exit 2
fi

errPath="/mnt/log/safekiddo/$program.err"
logPath="/mnt/log/safekiddo/$program.log"

echo "--- CRASH with error code $exit_code, date: $(date) ---" >> "$errPath"

reportDir="/mnt/log/safekiddo/CrashReport_$(date --utc "+%Y-%m-%d_%H%M%S")"

mkdir "$reportDir"

cp "$errPath" "$reportDir/"
cp "$logPath" "$reportDir/"

gzip -9 "$reportDir/"*
