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
    "uses-permission: android.permission.GET_ACCOUNTS",
    "uses-permission: com.google.android.c2dm.permission.RECEIVE",
    "permission: com.mapswithme.maps.pro.beta.permission.C2D_MESSAGE",
    "uses-permission: com.mapswithme.maps.pro.beta.permission.C2D_MESSAGE",
    "uses-permission: android.permission.VIBRATE",
    "permission: com.mapswithme.maps.pro.beta.permission.RECEIVE_ADM_MESSAGE",
    "uses-permission: com.mapswithme.maps.pro.beta.permission.RECEIVE_ADM_MESSAGE",
    "uses-permission: com.amazon.device.messaging.permission.RECEIVE",
}

logger = logging.getLogger()
logger.level = logging.DEBUG
logger.addHandler(logging.StreamHandler(sys.stdout))


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
        self.build_tools_version = self.get_build_tools_version()
        self.aapt = self.find_aapt()
        self.apk = self.find_apks()


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
        apk = filter(
            lambda s: "universal" in s and "unaligned" not in s,
            listdir(apk_path)
        )[0]
        apk_path = path.join(apk_path, apk)
        logging.info("Using apk at {}".format(apk_path))

        return apk_path


    def get_property(self, property, props_path):
        if not isfile(props_path):
            raise IOError("Properties file does not exist: {}".format(props_path))

        logging.info("local props: {}".format(props_path))
        with open(props_path) as props:
            for line in filter(lambda s: s != "" and not s.startswith("#"), map(lambda s: s.strip(), props)):
                if line.startswith(property):
                    return line.split("=")[1].strip()

        raise IOError("Couldn't find property {} in file {}".format(property, props_path))


    def find_aapt(self):
        local_props_path = path.join(self.omim_path, "android", "local.properties")
        sdk_path = self.get_property("sdk.dir", local_props_path)
        aapt_path = self.find_aapt_in_platforms(sdk_path)

        logging.info("Using aapt at {}".format(aapt_path))
        return aapt_path


    def find_aapt_in_platforms(self, platforms_path):
        pat = re.compile("[\.\-]")
        aapts = {}
        candidates = exec_shell("find", "{} -name aapt".format(platforms_path))
        for c in candidates:
            if "build-tools/{}".format(self.build_tools_version) in c:
                return c

            try:
                version = tuple(map(lambda s: int(s), pat.split(exec_shell(c, "v")[0][31:])))
                aapts[version] = c
            except:
                # Do nothing, because aapt version contains non-numeric symbols
                pass

        max_version = sorted(aapts.keys(), reverse=True)[0]
        logging.info("Max aapt version: {}".format(max_version))

        return aapts[max_version]


    def clean_up_permissions(self, permissions):
        ret = set()
        for p in permissions:
            if "name='" in p:
                p = p.replace("name='", "")
                p = p.replace("'", "")
            ret.add(p)
        return ret


    def test_permissions(self):
        """Check whether we have added or removed any permissions"""
        permissions = exec_shell(self.aapt, "dump permissions {0}".format(self.apk))
        permissions = set(
            filter(
                lambda s: s.startswith("permission:") or s.startswith("uses-permission:"),
                permissions
            )
        )

        permissions = self.clean_up_permissions(permissions)

        description = "Expected: {}\n\nActual: {}\n\nAdded: {}\n\nRemoved: {}".format(
            new_lines(EXPECTED_PERMISSIONS),
            new_lines(permissions),
            new_lines(permissions - EXPECTED_PERMISSIONS),
            new_lines(EXPECTED_PERMISSIONS - permissions)
        )

        self.assertEqual(EXPECTED_PERMISSIONS, permissions, description)


if __name__ == "__main__":
    unittest.main()
    exit(1)
