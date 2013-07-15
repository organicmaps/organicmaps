from random import randint
import os

from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice


# for notification bar
defYoffset = 50


class DeviceInfo:
    width = 0
    height = 0
    model = 'UnknownDevice'
    lang = 'NA'
    country = 'na'

    def __init__(self, device):
        self.width = int(device.getProperty('display.width'))
        self.height = int(device.getProperty('display.height'))
        self.model = str(device.getProperty('build.model'))
        self.lang = str(device.shell('getprop persist.sys.language')).rstrip()
        self.country = str(device.shell('getprop persist.sys.country')).rstrip()

    def dump(self):
        return '%s_%s_%s_%sx%sx' % (self.model, self.lang, self.country, self.height, self.width)

    def get_screenshot_dir(self):
        dirname = self.dump()
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        return dirname

    def get_screenshot_path(self, tag):
        dirname = self.get_screenshot_dir()
        return '%s/screenshot-%s.png' % (dirname, tag)


def is_still_running(device, package):
    curPack = str(device.getProperty('am.current.package'))
    print curPack
    return package == curPack


def swipe(device, start, end):
    device.drag(start, end, 0.01, 100)


def click(device, x, y):
    device.touch(x, y, 'DOWN_AND_UP')


def randswipe(device, di=None, count=1):
    if not di:
        di = DeviceInfo(device)
    for x in range(0, count):
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


def dumb_test(device, package):
    di = DeviceInfo(device)
    print 'Starting test at %s' % di.dump()
    MonkeyRunner.sleep(10)
    testSteps = 100
    for i in range(0, testSteps):
        if is_still_running(device, package):
            # atack!
            randswipe(device, di)
            randclick(device, di, 3)
            # Give some time to render new tiles
            MonkeyRunner.sleep(0.5)
            device.takeSnapshot().writeToFile(di.get_screenshot_path(i))
        else:
            print 'Where is MapsWithMe?'
    print 'Done testing %s' % di.dump()