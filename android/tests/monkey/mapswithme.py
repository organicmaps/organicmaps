# coding=utf-8
import time

from mutil import DeviceInfo
from com.android.monkeyrunner import MonkeyRunner

apkPath = '../MapsWithMeTest/bin/MapsWithMeTest-release.apk'
package = 'com.mapswithme.maps.pro'
activity = 'com.mapswithme.maps.DownloadResourcesActivity'
mapactivity = 'com.mapswithme.maps.MwmActivity'


def run_tasks(device, tasks):
    # wait for initialization
    MonkeyRunner.sleep(5)
    for t in tasks:
        t.send(device)


class MWMTask:
    def send(self, device):
        pass


class SearchTask(MWMTask):
    task = 'task_search'
    query = ''
    scope = 0

    def __init__(self, query, scope=0):
        self.query = query
        self.scope = scope


    def send(self, device):
        device.broadcastIntent(action='mwmtest',
                               extras={'task': self.task, 'search_query': str(self.query),
                                       'search_scope': str(self.scope)})
        print 'Sent', self
        MonkeyRunner.sleep(3)
        di = DeviceInfo(device)
        device.takeSnapshot().writeToFile(di.get_screenshot_path(self))


    def __str__(self):
        return 'search_%s_%s' % (self.query, str(self.scope))


class ShowGroupTask(MWMTask):
    task = 'task_bmk'
    name = ''
    duration = 0

    def __init__(self, name, duration):
        self.name = name
        self.duration = duration

    def send(self, device):
        di = DeviceInfo(device)
        # send intent
        device.broadcastIntent(action='mwmtest',
                               extras={'task': self.task, 'name': self.name})

        start = time.time()
        end = start + self.duration
        now = start
        print now, end, end - now
        while now < end:
            now = time.time()
            filename = di.get_screenshot_path('%s_%s' % (self, now))
            device.takeSnapshot().writeToFile(filename)
            print 'Captured', filename
            MonkeyRunner.sleep(0.7)


    def __str__(self):
        return 'group_show_%s_%s' % (self.name, str(self.duration))
