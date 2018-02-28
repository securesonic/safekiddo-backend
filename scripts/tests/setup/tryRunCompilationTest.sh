#!/bin/bash

# The following crontab entry is needed on webs1 on jenkins user:
# * * * * * ~/repos/9ld/scripts/tests/setup/tryRunCompilationTest.sh

cd ~/repos/9ld_mirror
OLD_HASH=$(git rev-parse devel)
git fetch
NEW_HASH=$(git rev-parse devel)

if [[ $OLD_HASH != "$NEW_HASH" ]]; then
	cd ~/repos/9ld
	git pull
	curl http://localhost:9080/job/CompilationTest/build
fi
