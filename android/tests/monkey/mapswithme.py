# coding=utf-8

from mutil import DeviceInfo
from com.android.monkeyrunner import MonkeyRunner

apkPath = '../MapsWithMeTest/bin/MapsWithMeTest-release.apk'
package = 'com.mapswithme.maps.pro'
activity = 'com.mapswithme.maps.DownloadResourcesActivity'
mapactivity = 'com.mapswithme.maps.MWMActivity'


def run_tasks(device, tasks):
    MonkeyRunner.sleep(5)
    di = DeviceInfo(device)
    for x in range(0, len(tasks)):
        print tasks[x]
        tasks[x].send(device)
        MonkeyRunner.sleep(5)
        device.takeSnapshot().writeToFile(di.get_screenshot_path(x))


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

    def __str__(self):
        return 'SearchTask %s %s' % (self.query, str(self.scope))
