#!/usr/bin/env python

import logging
import subprocess
import time
from argparse import ArgumentParser
from os.path import abspath, join

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


def get_args():
    parser = ArgumentParser(
        description="A test for checking that no new permissions have been added to the APK by a third library"
    )

    parser.add_argument(
        "-a", "--aapt-path",
        help="Path to the aapt executable.",
        dest="aapt", default=None
    )

    parser.add_argument(
        "-i", "--input-apk",
        help="Path to the apk to test.",
        dest="apk", default=None
    )

    return parser.parse_args()


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


def new_lines(iterable):
    return "\n".join(iterable)


def find_aapt(args):
    my_path = abspath(__file__)
    omim_path = my_path[:my_path.index("omim") + len("omim")]
    local_props_path = join(omim_path, "android", "local.properties")
    logging.info("local props: {}".format(local_props_path))
    with open(local_props_path) as props:
        for line in filter(lambda s: s != "" and not s.startswith("#"), map(lambda s: s.strip(), props)):
            if line.startswith("sdk.dir"):
                path = line.split("=")[1].strip()
                args.aapt = join(path, "platforms", "android-6", "tools", "aapt")


def check_permissions(permissions):
    if permissions != EXPECTED_PERMISSIONS:
        logging.info("\nExpected:\n{0}".format(new_lines(EXPECTED_PERMISSIONS)))
        logging.info("\nActual: \n{0}".format(new_lines(permissions)))
        logging.info(
            "\nAdded:\n{0}".format(
                new_lines(permissions - EXPECTED_PERMISSIONS)
            )
        )
        logging.info(
            "\nRemoved:\n{0}".format(
                new_lines(EXPECTED_PERMISSIONS - permissions)
            )
        )
        logging.info("FAILED")
        return False

    logging.info("OK")
    return True


if __name__ == "__main__":
    start_time = time.time()
    logging.basicConfig(level=logging.DEBUG, format="%(msg)s")
    logging.info("Running {0}".format(__file__))

    args = get_args()

    if args.aapt is None:
        find_aapt(args)

    logging.info("Using aapt at: {}".format(args.aapt))

    permissions = exec_shell(args.aapt, "dump permissions {0}".format(args.apk))
    permissions = set(
        filter(
            lambda s: s.startswith("permission:") or s.startswith("uses-permission:"),
            permissions
        )
    )

    check = check_permissions(permissions)
    logging.info("Test took {} ms".format((time.time() - start_time) * 1000))
    if not check:
        exit(1)


