#!/bin/bash

set -o errexit

cd "$WORKSPACE"
rm -f latency.txt performance.txt timeouts.txt

cd $JENKINS_HOME/repos/9ld
REMOTE_HOST="tst-sk-ws-func"

COMMAND=$(./scripts/tests/setup/runMe.py)

ssh $REMOTE_HOST "$COMMAND"

cd "$WORKSPACE"

ssh $REMOTE_HOST 'cat lastTestDir/latency.txt' > latency.txt
ssh $REMOTE_HOST 'cat lastTestDir/performance.txt' > performance.txt
ssh $REMOTE_HOST 'cat lastTestDir/timeouts.txt' > timeouts.txt
