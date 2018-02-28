#!/bin/bash

set -o errexit

$JENKINS_HOME/repos/9ld/scripts/tests/externalPerformanceTest/externalPerformanceTest.py \
	--httpdHost='TST-SK-WS-LB-565849150.eu-west-1.elb.amazonaws.com' \
	--urls=100 \
	--stepSize=50
