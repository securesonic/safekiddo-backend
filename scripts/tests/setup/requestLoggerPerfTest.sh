#!/bin/bash

set -o errexit

cd "$WORKSPACE"
rm -f plot.txt

cd $JENKINS_HOME/repos/9ld
REMOTE_HOST="tst-sk-ws-func"

COMMAND=$(./scripts/tests/setup/runMe.py)

ssh $REMOTE_HOST "$COMMAND"

cd "$WORKSPACE"

PERF_16=`ssh $REMOTE_HOST 'cat lastTestDir/performance_16.txt'`
PERF_1=`ssh $REMOTE_HOST 'cat lastTestDir/performance_1.txt'`

echo -e "$REMOTE_HOST 16 workers,$REMOTE_HOST 1 worker\n$PERF_16,$PERF_1" > plot.txt
