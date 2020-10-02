#!/usr/bin/env python
import os
import sys

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", ".."))

from data.base import get_version
from data.base import setup

_V = get_version()

_D = [
    "omim-data-borders",
    "omim-data-essential",
    "omim-data-files",
    "omim-data-fonts",
    "omim-data-styles",
]

setup(__file__, "all", [], install_requires=["{}=={}".format(d, _V) for d in _D])
