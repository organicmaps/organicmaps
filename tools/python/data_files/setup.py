#!/usr/bin/env python3
import os
import sys

import setuptools

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", "..", ".."))

from pyhelpers.setup import chdir
from pyhelpers.setup import get_version


with chdir(os.path.abspath(os.path.dirname(__file__))):
    supported_pythons = ("2", "2.7", "3", "3.5", "3.6", "3.7")
    setuptools.setup(
        name="omim-data-files",
        version=str(get_version()),
        author="CoMaps",
        author_email="info@comaps.app",
        description="This package is a library for dealing with data files.",
        url="https://codeberg.org/comaps",
        package_dir={"data_files": ""},
        packages=["data_files",],
        classifiers=["License :: OSI Approved :: Apache Software License",]
        + [
            "Programming Language :: Python :: {}".format(supported_python)
            for supported_python in supported_pythons
        ],
    )
