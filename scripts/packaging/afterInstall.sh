#!/bin/sh

chown -R safekiddo:safekiddo /sk/bin/
chown -R safekiddo:safekiddo /sk/scripts/

umask 022 # SAF-527

mkdir -p /sk/run/safekiddo
chown -R safekiddo:safekiddo /sk/run/safekiddo

mkdir -p /mnt/log/safekiddo
chown -R safekiddo:safekiddo /mnt/log/safekiddo

chmod 600 /sk/bin/pgpass
chmod 644 /etc/logrotate.d/safekiddo

su safekiddo -c '/sk/bin/configureBackend.py'

# monit configuration update
service monit restart

# restart all services
monit restart all
