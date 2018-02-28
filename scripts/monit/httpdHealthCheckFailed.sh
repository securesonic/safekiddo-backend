#!/bin/bash

echo "[$(date)] httpd health check failed, stopping backend" >> "/mnt/log/safekiddo/backend.err"
/sk/scripts/monit/stopBackend.sh
