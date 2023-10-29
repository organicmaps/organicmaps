#!/usr/bin/env python3

"""
    Run from `$OMIM_ROOT/tools/python/test_server` dir
"""

import subprocess

SERVER_RUNNABLE = 'server/testserver.py'
LOG_FILE = 'server.log'

with open(LOG_FILE, 'w') as log:
    process = subprocess.Popen(['python3', SERVER_RUNNABLE], stdout=subprocess.DEVNULL, stderr=log, text=True)

while True:
    with open(LOG_FILE, 'r') as log:
        data = log.read()
        if data and "Started server with pid" in data:
            break
