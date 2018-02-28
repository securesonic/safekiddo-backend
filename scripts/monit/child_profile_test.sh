#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
cd "$SCRIPT_DIR"

source configuration.sh

export PGPASSFILE=/sk/bin/pgpass

psql -U backend -h "$CONFIG_DB_HOST" -d skcfg -f ../../sql/check_child_profile_for_icap.sql > /tmp/actual_child_profile_for_icap.out
diff -U10 /tmp/actual_child_profile_for_icap.out reference_child_profile_for_icap.out
