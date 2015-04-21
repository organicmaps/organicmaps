#!/usr/bin/env python

"""
This script is for running the Android tests on android devices from the build 
server. It may take as a parameter the build number (e.g. the build number from 
Jenkins). The build number may also be a string.

The script removes all the apps whose ids contain "mapswithme", and cleans up 
the device logs before running the tests, so make sure you don't have important 
logs on devices before you connect them to the server. After the test, the 
device logs are filtered and saved as <build number>_<device_id>.log in the 
current directory.
"""


from __future__ import print_function
import re
import subprocess
import time
import sys
import threading
import os
from os import listdir
from os.path import isfile, join


# this list is for removing possible MapsWithMe data folders, not used yet, but will be in the future
# mapswithme_paths=["storage/sdcard0/MapsWithMe", "mnt/sdcard/MapsWithMe", 
#                   "/mnt/shell/emulated/0/MapsWithMe", "/mnt/sdcard/MapsWithMe"]


ACTIVITY_NAME = "com.mapswithme.maps.unittests.debug/com.mapswithme.maps.unittests.AllTestsActivity"

APP_ID = "com.mapswithme.maps.unittests.debug"

APK_LOCATION = "build/outputs/apk/UnitTests-debug.apk"

MAX_TEST_RUN_TIME = 3600 # seconds

TEST_WAIT_PAUSE = 2 # seconds

SERIAL_PATTERN = re.compile("^[\da-f]{4,}")

build_number = "0"


def exec_shell(command):
    # print "> " + command
    s = subprocess.check_output(command.split())
    return s.split('\r\n')


def adb(command, serial=None, shell=False):

    shell_cmd = list(['adb'])
    if serial is not None:
        shell_cmd += ['-s', serial]
    if shell:
        shell_cmd += ['shell']
    shell_cmd += [command]

    return exec_shell(' '.join(shell_cmd).replace("  ", " "))


def uninstall(serial, package):
    adb("pm uninstall -k {}".format(package), serial=serial, shell=True)


def install(serial, path):
    command = " -s {serial} install {path}".format(serial=serial, path=path)
    adb(command)


def connected_devices():
    adb_devices = adb("devices")
    filter_fn = lambda device : SERIAL_PATTERN.match(device)
    map_fn = lambda d : d.split("\t")[0]

    return [ id for id in process_output(adb_devices, map_fn, filter_fn)]


def process_output(output, fn, filter_fn):
    """
    adb returns an array of strings, at least one of them contains several lines of output.
    To all probability, only the 0th string in the array contains any meaningful output, 
    but there is some chance that others might as well, so to be on the safe side, we
    process all of them.
    """
    for full_string in output:
        lines = full_string.split("\n")

        for line in lines:
            if not filter_fn(line):
                continue

            yield fn(line)


def packages(serial):
    packs = adb("pm list packages | grep mapswithme", serial=serial, shell=True)

    filter_fn = lambda x : x.startswith("package:")
    ret_fn = lambda x : x.split(":")[1]

    return process_output(packs, ret_fn, filter_fn)


def app_is_running(serial):
    # if the app is not running, we get just an empty line, otherwise we get
    # the app info line + the empty line.  This lets us assume that the app is not running.
    command = "ps | grep {}".format(APP_ID)
    result = adb(command, serial=serial, shell=True)
    return len(result) > 1


def run_app(serial):
    command = "am start -n {}".format(ACTIVITY_NAME)
    adb(command, serial=serial, shell=True)


def save_log(serial):
    command = "logcat -d | grep MapsMeTest"
    device_log = adb(command, serial=serial)
    lines = process_output(device_log, lambda x: x, lambda x: True)
    write_lines_to_file(lines, serial)


def write_lines_to_file(lines, serial):
    with open("{build_number}_{serial}.log".format(build_number=build_number, serial=serial), "w") as log_file:
        for line in lines:
            log_file.write(line + "\n")



def clear_log(serial):
    command = "-s {serial} logcat -c".format(serial=serial)
    adb(command)


def device_run_loop(serial):
    start = time.time()

    clear_log(serial)
    run_app(serial)

    elapsed_time = 0

    while elapsed_time < MAX_TEST_RUN_TIME:
        if not app_is_running(serial):
            break

        time.sleep(TEST_WAIT_PAUSE)
        elapsed_time += TEST_WAIT_PAUSE

    if elapsed_time >= MAX_TEST_RUN_TIME:
        print("The tests on {serial} took too long".format(serial=serial))

    save_log(serial)

    end = time.time()
    print("Ran tests on {serial} in {duration} sec.".format(serial=serial, duration=(end - start)))


def clean_device(serial):
    start = time.time()
    for pack in packages(serial):
        uninstall(serial, pack)

    install(serial, APK_LOCATION)
    end = time.time()
    print("Cleaned up {serial} in {duration} sec.".format(serial=serial, duration=(end - start)))


def process_devices(device_ids, fn):
    run_loop_threads = []

    for serial in device_ids:
        thread = threading.Thread(target=fn, args=(serial,))
        run_loop_threads.append(thread)
        thread.start()

    for thread in run_loop_threads:
        thread.join()    


def main():
    logs = [ f for f in listdir(".") if f.endswith(".log") and isfile(join(".",f)) ]

    for log in logs:
        os.remove(log)

    if len(sys.argv) > 1:
        global build_number
        build_number = sys.argv[1]

    device_ids = connected_devices()
    print("Running on devices:")
    for device_id in device_ids:
        print(device_id)
    
    print("\nCleaning up devices and installing test apk...")
    process_devices(device_ids, clean_device)
    print("\nRunning the test suites...")
    process_devices(device_ids, device_run_loop)

    print("\nTests finished running on all devices")


if __name__ == "__main__":
    main()
