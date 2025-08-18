#!/usr/bin/env python3

import subprocess

SERVER_RUNNABLE = 'server/testserver.py'

subprocess.Popen(['python3', SERVER_RUNNABLE], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
