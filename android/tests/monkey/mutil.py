from com.android.monkeyrunner import MonkeyRunner, MonkeyDevice
from random import randint, random

def test(device, snapshotname):
    print 'Starting test at %s' % device
    MonkeyRunner.sleep(5)
    for i in range(0,20):
        MonkeyRunner.sleep(0.2)
        # atack!
        rswipe(device)
        rclick(device)
        # Give some time to render new tiles
        MonkeyRunner.sleep(0.4)
        result = device.takeSnapshot()
        result.writeToFile(snapshotname  % i)
    print 'Done testing %s' % device


def swipe(device, start, end):
    device.drag(start, end, 0.01, 100)


def click(device,x,y):
    device.touch(x,y, 'DOWN_AND_UP')


def rswipe(device):
    step = 400
    start = (200, 400)
    dX = step*randint(-4,4)
    dY = step*randint(-4,4)
    end = (start[0] + dX, start[1] + dY)
    swipe(device, start, end)


def rclick(device):
    x = 800*random() 
    y = 1000*random() + 100
    click(device, x, y)
