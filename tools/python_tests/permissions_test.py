#!/usr/bin/env python

import logging
import re
import subprocess
import sys
import unittest
from os import listdir, path
from os.path import abspath, dirname, isfile

EXPECTED_PERMISSIONS = {
    "uses-permission: android.permission.WRITE_EXTERNAL_STORAGE",
    "uses-permission: android.permission.ACCESS_COARSE_LOCATION",
    "uses-permission: android.permission.ACCESS_FINE_LOCATION",
    "uses-permission: android.permission.INTERNET",
    "uses-permission: android.permission.ACCESS_WIFI_STATE",
    "uses-permission: android.permission.CHANGE_WIFI_STATE",
    "uses-permission: android.permission.ACCESS_NETWORK_STATE",
    "uses-permission: android.permission.WAKE_LOCK",
    "uses-permission: android.permission.BATTERY_STATS",
    "uses-permission: com.google.android.c2dm.permission.RECEIVE",
    "permission: com.mapswithme.maps.pro.permission.C2D_MESSAGE",
    "uses-permission: com.mapswithme.maps.pro.permission.C2D_MESSAGE",
    "uses-permission: android.permission.VIBRATE",
    "permission: com.mapswithme.maps.pro.permission.RECEIVE_ADM_MESSAGE",
    "uses-permission: com.mapswithme.maps.pro.permission.RECEIVE_ADM_MESSAGE",
    "uses-permission: com.amazon.device.messaging.permission.RECEIVE",
}

SPLIT_RE = re.compile("[\.\-]")
APK_RE = re.compile(".*universal(?!.*?unaligned).*$")
PROP_RE = re.compile("(?!\s*?#).*$")
CLEAN_PERM_RE = re.compile("(name='|'$|debug\.|beta\.)", re.MULTILINE)
BUILD_TYPE = r = re.compile(".*(debug|release|beta).*")

AAPT_VERSION_PREFIX_LEN = len("Android Asset Packaging Tool, v")


logger = logging.getLogger()
logger.level = logging.DEBUG
logger.addHandler(logging.StreamHandler(sys.stdout))


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
        self.aapt = self.find_aapt(self.get_build_tools_version())
        self.apk = self.find_apks()
        self.permissions = self.get_actual_permissions()
        if self.get_build_type() == "debug":
            logging.info("The build type is DEBUG")
            global EXPECTED_PERMISSIONS
            EXPECTED_PERMISSIONS.add("uses-permission: android.permission.READ_LOGS")

        self.failure_description = self.get_failure_description()

    def get_build_type(self):
        apk_name = path.basename(self.apk)
        return BUILD_TYPE.findall(apk_name)[0]


    def get_build_tools_version(self):
        return self.get_property(
            "propBuildToolsVersion",
            path.join(self.omim_path, "android", "gradle.properties")
        )


    def contains_correct_files(self, my_path):
        dir_contents = listdir(my_path)
        return (
            "strings.txt" in dir_contents or
            "android" in dir_contents or
            "drape_frontend" in dir_contents
        )


    def find_omim_path(self):
        my_path = abspath(path.join(dirname(__file__), "..", ".."))
        if self.contains_correct_files(my_path):
            return my_path
        raise IOError("Couldn't find a candidate for the project root path")


    def find_apks(self):
        apk_path = path.join(self.omim_path, "android", "build", "outputs", "apk")
        on_disk = listdir(apk_path)
        print(on_disk)
        apks = filter(APK_RE.match, on_disk)
        if not apks:
            raise IOError("Couldn't find an APK")
        apk_path = path.join(apk_path, apks[0])
        logging.info("Using apk at {}".format(apk_path))

        return apk_path


    def get_property(self, prop, props_path):
        if not isfile(props_path):
            raise IOError("Properties file does not exist: {}".format(props_path))

        logging.info("local props: {}".format(props_path))
        with open(props_path) as props:
            for line in filter(PROP_RE.match, props):
                if line.startswith(prop):
                    return line.split("=")[1].strip()

        raise IOError("Couldn't find property {} in file {}".format(prop, props_path))


    def find_aapt(self, build_tools_version):
        local_props_path = path.join(self.omim_path, "android", "local.properties")
        sdk_path = self.get_property("sdk.dir", local_props_path)
        aapt_path = self.find_aapt_in_platforms(sdk_path, build_tools_version)

        logging.info("Using aapt at {}".format(aapt_path))
        return aapt_path


    def find_aapt_in_platforms(self, platforms_path, build_tools_version):
        aapts = {}
        candidates = exec_shell("find", "{} -name aapt".format(platforms_path))
        for c in candidates:
            if "build-tools/{}".format(build_tools_version) in c:
                return c

            try:
                version = tuple(map(int, self.get_aapt_version(c)))
                aapts[version] = c
            except ValueError:
                # Do nothing, because aapt version contains non-numeric symbols
                pass

        max_version = sorted(aapts.iterkeys(), reverse=True)[0]
        logging.info("Max aapt version: {}".format(max_version))

        return aapts[max_version]


    def get_aapt_version(self, candidate):
        return SPLIT_RE.split(exec_shell(candidate, "v")[0][AAPT_VERSION_PREFIX_LEN:])


    def get_actual_permissions(self):
        permissions = exec_shell(self.aapt, "dump permissions {0}".format(self.apk))
        permissions = filter(
            lambda s: s.startswith("permission:") or s.startswith("uses-permission:"),
            permissions
        )

        return set(
            map(lambda s: CLEAN_PERM_RE.sub("", s), permissions)
        )


    def get_failure_description(self):
        join_by_new_lines = lambda iterable: "\n".join(sorted(iterable))

        return "Expected:\n{}\n\nActual:\n{}\n\nAdded:\n{}\n\nRemoved:\n{}".format(
            join_by_new_lines(EXPECTED_PERMISSIONS),
            join_by_new_lines(self.permissions),
            join_by_new_lines(self.permissions - EXPECTED_PERMISSIONS),
            join_by_new_lines(EXPECTED_PERMISSIONS - self.permissions)
        )


    def test_permissions(self):
        """Check whether we have added or removed any permissions"""
        self.assertEqual(EXPECTED_PERMISSIONS, self.permissions, self.failure_description)


if __name__ == "__main__":
    unittest.main()
    exit(1)
