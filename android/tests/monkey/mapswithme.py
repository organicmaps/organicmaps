# coding=utf-8

from mutil import DeviceInfo
from com.android.monkeyrunner import MonkeyRunner

apkPath = '../MapsWithMeTest/bin/MapsWithMeTest-release.apk'
package = 'com.mapswithme.maps.pro'
activity = 'com.mapswithme.maps.DownloadResourcesActivity'
mapactivity = 'com.mapswithme.maps.MWMActivity'

sampleUris = ([
                  ('ge0://o4aXJl2FbJ/TNT_Rock_Club', 0),
                  ('ge0://o4aXJlDxJj/Newman', 1),
                  ('ge0://Q4CNu8AejK/Paris', 2),
                  ('ge0://04aXJfyIVr/Опоп-69', 3),
                  ('ge0://jwDj9dKFgF/Port_Harcourt', 4)
              ])


def follow_path(device, path):
    MonkeyRunner.sleep(10)
    for x in range(0, len(path)):
        print path[x]
        sendUrl(device, path[x][0], path[x][1])


def sendUrl(device, geoUri, index=0):
    device.broadcastIntent(action='mwmtest', extras={'uri': geoUri})
    MonkeyRunner.sleep(1)
    device.wake()
    di = DeviceInfo(device)
    device.takeSnapshot().writeToFile(di.get_screenshot_path(index))