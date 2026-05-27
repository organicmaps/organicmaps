#!/usr/bin/env python3

import sys
import urllib.error
import urllib.request

from server.config import PORT

URL = f'http://localhost:{PORT}/kill'

try:
    with urllib.request.urlopen(URL, timeout=5.0) as response:
        response.read()
except urllib.error.URLError as ex:
    # Not a hard error: the server may already be gone or never started.
    print(f"stop_server.py: {URL} -> {ex}", file=sys.stderr)
