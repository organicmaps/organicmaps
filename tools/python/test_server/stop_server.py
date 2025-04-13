#!/usr/bin/env python3

import subprocess

from server.config import PORT

subprocess.call(['curl', f'http://localhost:{PORT}/kill'])
