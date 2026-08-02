#!/usr/bin/env python3

import socket
import sys
import time
import urllib.error
import urllib.request

from server.config import PORT

HOST = 'localhost'
URL = f'http://{HOST}:{PORT}/kill'
TIMEOUT_SECONDS = 15.0
POLL_INTERVAL_SECONDS = 0.05

try:
    with urllib.request.urlopen(URL, timeout=5.0) as response:
        response.read()
except urllib.error.URLError as ex:
    # Not a hard error: the server may already be gone or never started.
    print(f"stop_server.py: {URL} -> {ex}", file=sys.stderr)

# /kill only asks the server to shut down; it keeps the listening socket until its request
# loop unwinds. Returning before that makes the next start_server.py fail with "Address
# already in use", which is what back-to-back ctest runs hit.
deadline = time.perf_counter() + TIMEOUT_SECONDS
while time.perf_counter() < deadline:
    with socket.socket() as probe:
        probe.settimeout(POLL_INTERVAL_SECONDS)
        if probe.connect_ex((HOST, PORT)) != 0:
            sys.exit(0)
    time.sleep(POLL_INTERVAL_SECONDS)

print(f"stop_server.py: port {PORT} is still in use after {TIMEOUT_SECONDS}s", file=sys.stderr)
