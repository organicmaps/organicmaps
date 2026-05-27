#!/usr/bin/env python3

import http.client
import os
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request

from pathlib import Path

from server.config import PORT

SCRIPT_DIR = Path(os.path.dirname(__file__))
SERVER_RUNNABLE = SCRIPT_DIR / 'server/testserver.py'

HOST = 'localhost'
# /id returns the server PID without mutating its internal client counter.
# Using /ping here would inflate that counter and break /kill-triggered
# shutdown from stop_server.py, leaving a zombie server between runs.
READY_URL = f'http://{HOST}:{PORT}/id'
TIMEOUT_SECONDS = 15.0
POLL_INTERVAL_SECONDS = 0.05

log_fd, log_path = tempfile.mkstemp(prefix='testserver-', suffix='.log')
log_file = os.fdopen(log_fd, 'w')
proc = subprocess.Popen(
    [sys.executable, '-u', str(SERVER_RUNNABLE)],
    stdout=log_file,
    stderr=subprocess.STDOUT,
)
log_file.close()  # The child inherits its own fd copy.


def dump_server_log(message):
    print(message, file=sys.stderr)
    print(f"--- testserver.py output ({log_path}) ---", file=sys.stderr)
    try:
        with open(log_path, 'r') as f:
            content = f.read()
    except OSError as read_err:
        content = f"<unable to read log file: {read_err}>"
    print(content if content else "<empty>", file=sys.stderr)
    print("--- end testserver.py output ---", file=sys.stderr)
    if proc.poll() is not None:
        print(f"testserver.py exited with code {proc.returncode}", file=sys.stderr)


# Wait until testserver.py is actually serving HTTP requests. A raw TCP
# connection check is insufficient: the kernel may accept connections on
# the listening socket before Python's request loop is running, causing
# the first real HTTP call to race with server startup.
start_time = time.perf_counter()
last_error = None
while True:
    if proc.poll() is not None:
        dump_server_log(f"testserver.py exited prematurely (code {proc.returncode})")
        sys.exit(1)

    try:
        with urllib.request.urlopen(READY_URL, timeout=1.0) as response:
            body = response.read().decode('ascii', errors='replace').strip()
            if response.status == 200 and body == str(proc.pid):
                break
            # Another server is answering on our port - our subprocess likely
            # failed to bind. Record and keep polling until our subprocess
            # exits (handled by proc.poll() above) or we time out.
            last_error = (
                f"/id returned pid {body!r}, expected {proc.pid} "
                f"(another server bound to {PORT}?)"
            )
    except (urllib.error.URLError, TimeoutError, OSError, http.client.HTTPException) as ex:
        last_error = ex

    if time.perf_counter() - start_time >= TIMEOUT_SECONDS:
        dump_server_log(
            f"Timed out after {TIMEOUT_SECONDS:.1f}s waiting for {READY_URL} "
            f"(last error: {last_error})"
        )
        try:
            proc.terminate()
        except OSError:
            pass
        sys.exit(1)

    time.sleep(POLL_INTERVAL_SECONDS)

print(f"testserver.py pid={proc.pid} ready; log: {log_path}")
