#!/bin/bash

SCRIPT_DIR=$(dirname "$(readlink -f "$0")")
"$SCRIPT_DIR/backend.sh" &

# Wait for backend to load local webroot database - worker threads are started after this.
# The waiting is necessary because Monit performs a health check by sending a query to WS, so
# we must first ensure backend is ready to handle requests from WS.
sleep 20
