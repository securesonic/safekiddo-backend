#!/bin/bash

set -o errexit

cd $JENKINS_HOME/repos/9ld
REMOTE_HOST="tst-sk-ws-func"

COMMAND=$(./scripts/tests/setup/runMe.py)

ssh $REMOTE_HOST "$COMMAND"
