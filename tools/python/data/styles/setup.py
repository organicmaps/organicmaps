#!/usr/bin/env python3
import os
import sys

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", ".."))

from data.base import get_version
from data.base import setup

setup(
    __file__,
    "styles",
    [
        "drules_proto.bin",
        "drules_proto_default_light.bin",
        "drules_proto_default_light.txt",
        "drules_proto_default_dark.bin",
        "drules_proto_default_dark.txt",
        "drules_proto_vehicle_light.bin",
        "drules_proto_vehicle_light.txt",
        "drules_proto_vehicle_dark.bin",
        "drules_proto_vehicle_dark.txt",
    ],
    install_requires=["omim-data-files=={}".format(get_version())]
)
