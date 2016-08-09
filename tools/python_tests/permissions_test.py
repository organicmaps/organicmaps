#!/usr/bin/env python

import unittest

import logging
import subprocess
from os.path import abspath, join, dirname
from os import listdir

EXPECTED_PERMISSIONS = {
    "uses-permission: android.permission.WRITE_EXTERNAL_STORAGE",
    "uses-permission: android.permission.ACCESS_COARSE_LOCATION",
    "uses-permission: android.permission.ACCESS_FINE_LOCATION",
    "uses-permission: android.permission.INTERNET",
    "uses-permission: android.permission.ACCESS_WIFI_STATE",
    "uses-permission: android.permission.CHANGE_WIFI_STATE",
    "uses-permission: android.permission.ACCESS_NETWORK_STATE",
    "uses-permission: android.permission.WAKE_LOCK",
    "uses-permission: android.permission.GET_ACCOUNTS",
    "uses-permission: com.google.android.c2dm.permission.RECEIVE",
    "permission: com.mapswithme.maps.pro.beta.permission.C2D_MESSAGE",
    "uses-permission: com.mapswithme.maps.pro.beta.permission.C2D_MESSAGE",
    "uses-permission: android.permission.VIBRATE",
    "permission: com.mapswithme.maps.pro.beta.permission.RECEIVE_ADM_MESSAGE",
    "uses-permission: com.mapswithme.maps.pro.beta.permission.RECEIVE_ADM_MESSAGE",
    "uses-permission: com.amazon.device.messaging.permission.RECEIVE",
}


def new_lines(iterable):
    return "\n".join(iterable)


def exec_shell(executable, flags):
    spell = ["{0} {1}".format(executable, flags)]
    logging.info("Spell: {}".format(spell[0]))
    process = subprocess.Popen(
        spell,
        stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        shell=True
    )

    out, _ = process.communicate()

    return filter(None, out.splitlines())


class TestPermissions(unittest.TestCase):
    def setUp(self):
        self.omim_path = self.find_omim_path()
        self.aapt = self.find_aapt()
        self.apk = self.find_apks()


    def contains_correct_files(self, my_path):
        dir_contents = listdir(my_path)
        return (
            "strings.txt" in dir_contents or
            "android" in dir_contents or
            "drape_frontend" in dir_contents
        )


    def find_omim_path(self):
        my_path = abspath(join(dirname(__file__), "..", ".."))
        if self.contains_correct_files(my_path):
            return my_path
        raise IOError("Couldn't find a candidate for the project root path")


    def find_apks(self):
        apk_path = join(self.omim_path, "android", "build", "outputs", "apk")
        apk = filter(
            lambda s: "universal" in s and "unaligned" not in s,
            listdir(apk_path)
        )[0]
        return join(apk_path, apk)


    def find_aapt(self):
        local_props_path = join(self.omim_path, "android", "local.properties")
        logging.info("local props: {}".format(local_props_path))
        with open(local_props_path) as props:
            for line in filter(lambda s: s != "" and not s.startswith("#"), map(lambda s: s.strip(), props)):
                if line.startswith("sdk.dir"):
                    path = line.split("=")[1].strip()
                    return join(path, "platforms", "android-6", "tools", "aapt")
        return None


    def test_permissions(self):
        """Check whether we have added or removed any permissions"""
        permissions = exec_shell(self.aapt, "dump permissions {0}".format(self.apk))
        permissions = set(
            filter(
                lambda s: s.startswith("permission:") or s.startswith("uses-permission:"),
                permissions
            )
        )

        description = "Expected: {}\n\nActual: {}\n\n:Added: {}\n\nRemoved: {}".format(
            new_lines(EXPECTED_PERMISSIONS),
            new_lines(permissions),
            new_lines(permissions - EXPECTED_PERMISSIONS),
            new_lines(EXPECTED_PERMISSIONS - permissions)
        )

        self.assertEqual(EXPECTED_PERMISSIONS, permissions, description)


if __name__ == "__main__":
    unittest.main()
    exit(1)
