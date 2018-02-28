#!/bin/bash

cd "$(dirname "$0")"
# FIXME: 
if [[ `hostname -s` = beta* ]]; then
	. /home/zytka/virtualenv-1.11.4/myVirtualPython/bin/activate
fi
if [[ `hostname -s` = webs* ]]; then
	. /var/lib/jenkins/virtualenv-1.11.4/safekiddoVE/bin/activate
fi
numProc=$1
shift
for i in `seq $numProc`; do
	python stressTest.py $@ &
done
wait
