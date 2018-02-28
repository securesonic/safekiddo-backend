#!/bin/bash

set -o errexit
# uncomment for debugging
#set -x

if [[ $# -ne 1 ]]; then
	echo "Usage: $(basename "$0") shell_script"
	echo
	echo -e "shell_script\tcommands needed to execute a script. May include newlines, quotes,"
	echo -e "\t\tvariable expansion, etc."
	exit 1
fi >&2

command=$1

buildsDir="/mnt/builds"
cd $buildsDir
compilationSubdir=$(cat ~/lastCompilationSubdir)
executionSubdir="${BUILD_ID}_${JOB_NAME##*/}_${BUILD_NUMBER}"
cp -al $compilationSubdir $executionSubdir
cd $executionSubdir

# arrays cannot be exported so must be here
SK_DB_OPTIONS=( --dbHost ''  --dbUser '' --dbPassword '' )
SK_WEBROOT_CREDS=( --webrootOem ardura --webrootDevice prod_ardura_1 --webrootUid ardura_dmz1223p95 )

ulimit -c unlimited
ulimit -n 2000 # the maximum number of open file descriptors
ulimit -u 1500 # the maximum number of user processes

echo "=============================================================="
echo "Running test in directory: $(pwd)"
echo "Git tag: $(cat ~/lastCompilationTag)"
echo "Git commit: $(cat ~/lastCompilationCommit)"
echo "Compilation directory: $buildsDir/$compilationSubdir"
echo "=============================================================="

rm -f ~/lastTestDir

# execute the test
# subshell to stay in the same directory
(
eval "$command"
)

ln -s "$(pwd)" ~/lastTestDir

# test finished successfully (we have errexit)
# clean up webroot databases
webrootDbDir="bin/$CONFIG/bcdatabase"
if [[ -d $webrootDbDir ]]; then
	find "$webrootDbDir" -name "*.bin" -delete
fi

echo "Test SUCCESS"
