#!/bin/bash

set -o errexit

CONFIG="release_64"
BUILD_DIR="~/tmp"
REMOTE_HOST="tst-sk-ws-func"

COMMAND="
set -o errexit
# uncomment for debugging
#set -x

mkdir -p $BUILD_DIR
cd $BUILD_DIR
rm -rf 9ld
git clone -b devel ~/repos/9ld/
cd 9ld
make CONFIG=$CONFIG
rm -rf $BUILD_DIR/9ld
"

ssh $REMOTE_HOST "$COMMAND"
