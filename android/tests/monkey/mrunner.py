#!/usr/bin/env monkeyrunner

from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice
from random import randint, random
import mutil as mu
import sys
import os

outputdir = '../tests/monkey/output'
id = 'default'

if len(sys.argv) > 1:
    # using specified id
    id = sys.argv[1]
    snapshotname = 'snapshot_%d_' + id + '.png'
    print 'Connecting to device %s' % id
    device = MonkeyRunner.waitForConnection(5, id)
else:
    snapshotname = 'snapshot_%d.png'
    print 'Connecting ...'
    device = MonkeyRunner.waitForConnection(timeout=5)

if not device:
    sys.exit("Could not connect to  device")
else:
    print 'Connected'

apkPath = '../MapsWithMePro/bin/MapsWithMePro-debug.apk'
package = 'com.mapswithme.maps.pro'
activity = 'com.mapswithme.maps.DownloadResourcesActivity'

print 'Installing file %s' % apkPath
device.installPackage(apkPath)
device.wake()

runComponent = package + '/' + activity
print 'Starting activity %s' % activity
device.startActivity(component=runComponent)

mu.test(device, snapshotname)
