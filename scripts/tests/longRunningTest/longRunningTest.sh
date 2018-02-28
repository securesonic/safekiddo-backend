#!/bin/bash

# USAGE:
# ./longRunningTest.sh numProcesses numConnectionsPerProcess requestInterval --httpdHost host --httpdPort port 

function killAll { jobs -p | xargs kill; }

cd "$(dirname "$0")"
# FIXME:
if [[ `hostname -s` = beta48 ]]; then
	echo "beta48 has recent software, yeah!"
elif [[ `hostname -s` = beta* ]]; then
	. /home/zytka/virtualenv-1.11.4/myVirtualPython/bin/activate
elif [[ `hostname -s` = webs* ]]; then
	. /var/lib/jenkins/virtualenv-1.11.4/safekiddoVE/bin/activate
fi

numProc=$1
shift
for i in `seq $numProc`; do
	python longRunningTest.py $@ &
	pid[$i]=$!
done
trap killAll SIGINT
wait
