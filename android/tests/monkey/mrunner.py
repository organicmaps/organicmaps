#!/usr/bin/env monkeyrunner

import sys

from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice
from mapswithme import activity
import mapswithme as mwm


if len(sys.argv) > 1:
    # using specified id
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

apkPath = mwm.apkPath
activity = mwm.activity
package = mwm.package

print 'Installing file %s' % apkPath

device.installPackage(apkPath)
device.wake()

runComponent = package + '/' + activity
print 'Starting activity %s' % activity
device.startActivity(component=runComponent)

#mu.dumb_test(device, package)
mwm.follow_path(device, mwm.sampleUris)