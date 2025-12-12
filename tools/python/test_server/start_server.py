#!/usr/bin/env python3

import os
import socket
import subprocess
import sys
import time

from pathlib import Path

SCRIPT_DIR = Path(os.path.dirname(__file__))
SERVER_RUNNABLE = SCRIPT_DIR / 'server/testserver.py'

subprocess.Popen([sys.executable, SERVER_RUNNABLE], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)

# Now wait until the server actually starts.

HOST='localhost'
PORT=34568  # Should match config.py
TIMEOUT_SECONDS=3.0

# Source: https://gist.github.com/butla/2d9a4c0f35ea47b7452156c96a4e7b12
start_time = time.perf_counter()
while True:
    try:
        with socket.create_connection((HOST, PORT), timeout=TIMEOUT_SECONDS):
            break
    except OSError as ex:
        time.sleep(0.01)
        if time.perf_counter() - start_time >= TIMEOUT_SECONDS:
            raise TimeoutError(
                "Waited too long for the port {} on host {} to start accepting connections.".format(
                    PORT, HOST
                )
            ) from ex
