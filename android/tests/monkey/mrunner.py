#!/usr/bin/env monkeyrunner
# coding=utf-8

import sys

from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice
from mapswithme import *


sampleTasks = (SearchTask('Minsk'),
               SearchTask('London'),
               SearchTask('еда'),
               SearchTask('кино'),
               SearchTask('drink'),
               SearchTask('eat'),
               SearchTask('tener'),
               SearchTask('\"улица ленина\"'),
               SearchTask('Berlin'))

if len(sys.argv) > 1:
    id = sys.argv[1]
    print 'Connecting to device %s' % id
    device = MonkeyRunner.waitForConnection(5, id)
else:
    print 'Connecting ...'
    device = MonkeyRunner.waitForConnection(timeout=5)

if not device:
    sys.exit("Could not connect to  device")
else:
    print 'Connected'

print 'Installing file %s' % apkPath

device.installPackage(apkPath)

device.wake()
#mu.dumb_test(device, package)
run_tasks(device, sampleTasks)