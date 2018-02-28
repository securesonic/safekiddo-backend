#!/bin/bash

set -o errexit

isBranchName()
{
	local name=$1
	for branch in $(git for-each-ref --format='%(refname:short)' refs/heads/); do
		if [[ $branch = "$name" ]]; then
			return 0
		fi
	done
	return 1
}

cd $JENKINS_HOME/repos/9ld_mirror # cloned with --mirror option
REMOTE_HOST="tst-sk-ws-func"

if [[ $compilationTag = +* ]]; then
	# user requested to override fast-forward-push check
	# the tag is actually a branch name
	git push $REMOTE_HOST:repos/9ld/ "$compilationTag"
	# remove '+' to make valid tag
	compilationTag=${compilationTag#+}
elif ! git rev-parse --verify -q "$compilationTag^{commit}"; then
	echo "Tag '$compilationTag' does not name a commit!"
	exit 1
elif isBranchName "$compilationTag"; then
	# this part is necessary, because we want to update that particular branch
	# and not just one arbitrary branch (after else we use git branch --contains)
	git push $REMOTE_HOST:repos/9ld/ "$compilationTag"
else
	# choose one branch containing this commit
	branch=$(git branch --contains "$compilationTag" | head -1)
	# remove redundant whitespace
	branch=$(echo $branch)
	if [[ -z $branch ]]; then
		echo "Tag '$compilationTag' is not available on any branch!"
		exit 1
	fi
	# push just only that branch to include given commit
	git push $REMOTE_HOST:repos/9ld/ "$branch"
fi

COMMAND="
set -o errexit
# uncomment for debugging
#set -x

# save some variables here (expanded on webs1)
COMPILATION_SUBDIR=custom_$BUILD_NUMBER
CONFIG=debug_64
CHECKOUT_TAG=$compilationTag

# Note: script is read on webs1 side but variable references in it
# are expanded on $REMOTE_HOST
$(git show devel:scripts/tests/setup/compile.sh)
"

ssh $REMOTE_HOST "$COMMAND"
