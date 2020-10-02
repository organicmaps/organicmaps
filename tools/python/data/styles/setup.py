#!/usr/bin/env python
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
        "drules_proto-bw.bin",
        "drules_proto_clear.bin",
        "drules_proto_clear.txt",
        "drules_proto_dark.bin",
        "drules_proto_dark.txt",
        "drules_proto_vehicle_clear.bin",
        "drules_proto_vehicle_clear.txt",
        "drules_proto_vehicle_dark.bin",
        "drules_proto_vehicle_dark.txt",
    ],
    install_requires=["omim-data-files=={}".format(get_version())]
)
