#!/usr/bin/env monkeyrunner
# coding=utf-8

import sys

from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice
from mapswithme import *

enTask = [ShowGroupTask('Gr', 4 * 11), ShowGroupTask('Tha', 4 * 8)]

deTask = [ShowGroupTask('Ger', 4 * 11)]

ukTask = [ShowGroupTask('Uk', 4 * 9)]

spTask = [ShowGroupTask('Sp', 4 * 14)]

itTask = [ShowGroupTask('Italy', 4 * 10)]


# This one to run
sampleTasks = spTask


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
device.startActivity(package + '/' + activity)

device.wake()
# mu.dumb_test(device, package)
run_tasks(device, sampleTasks)