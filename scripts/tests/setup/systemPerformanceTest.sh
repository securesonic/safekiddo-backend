#!/bin/bash

set -o errexit

cd "$WORKSPACE"
rm -f plot.txt plot-ssl.txt

cd $JENKINS_HOME/repos/9ld
REMOTE_HOST="tst-sk-ws-func"

COMMAND=$(./scripts/tests/setup/runMe.py)

ssh $REMOTE_HOST "$COMMAND"

cd "$WORKSPACE"

PERFORMANCE=`ssh $REMOTE_HOST 'cat lastTestDir/performance.txt'`
PERFORMANCE_SSL=`ssh $REMOTE_HOST 'cat lastTestDir/performance-ssl.txt'`

echo -e "$REMOTE_HOST\n$PERFORMANCE" > plot.txt
echo -e "$REMOTE_HOST\n$PERFORMANCE_SSL" > plot-ssl.txt
