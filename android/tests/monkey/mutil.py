from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice
from random import randint, random

# for notification bar
defYoffset = 50


class DeviceInfo:
    width = 0
    height = 0
    model = 'UnknownDevice'

    def __init__(self, device):
        self.width = int(device.getProperty('display.width'))
        self.height = int(device.getProperty('display.height'))
        self.model = str(device.getProperty('build.model'))

    def dump(self):
        return '%s_%sx%sx' % (self.model, self.height, self.width)


def test(device, snapshotname):
    di = DeviceInfo(device)
    print 'Starting test at %s' % di.dump()
    MonkeyRunner.sleep(10)
    testSteps = 100
    for i in range(0, testSteps):
        # atack!
        randswipe(device, di)
        randclick(device, di, 3)
        # Give some time to render new tiles
        MonkeyRunner.sleep(0.3)
        device.takeSnapshot().writeToFile(snapshotname % i)
    print 'Done testing %s' % di.dump()


def swipe(device, start, end):
    device.drag(start, end, 0.01, 100)


def click(device, x, y):
    device.touch(x, y, 'DOWN_AND_UP')


def randswipe(device, di=None, count=1):
    if not di:
        di = DeviceInfo(device)
    for x in range(0, count):
        step = randint(di.width, di.height)
        start = (randint(0, di.width), randint(defYoffset, di.height))
        end = (randint(0, di.width), randint(defYoffset, di.height))
        print 's=%s e=%s' % (start, end)
        swipe(device, start, end)


def randclick(device, di=None, count=1):
    if not di:
        di = DeviceInfo(device)
    for x in range(0, count):
        x = randint(0, di.width)
        y = randint(defYoffset, di.height)
        print 'x=%d y=%d' % (x, y)
        click(device, x, y)
