#!/bin/bash

set -o errexit

cd $JENKINS_HOME/repos/9ld
REMOTE_HOST="tst-sk-ws-func"

# git pull was already done by tryRunCompilationTest.sh
git push $REMOTE_HOST:repos/9ld/ devel

COMMAND="
set -o errexit
# uncomment for debugging
#set -x

# save some variables here (expanded on webs1)
COMPILATION_SUBDIR=$BUILD_NUMBER
CONFIG=debug_64
CHECKOUT_TAG=devel

# Note: script is read on webs1 side but variable references in it
# are expanded on $REMOTE_HOST
$(< scripts/tests/setup/compile.sh)
"

ssh $REMOTE_HOST "$COMMAND"
