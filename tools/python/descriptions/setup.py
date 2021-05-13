#!/usr/bin/env python
import os
import sys

import setuptools

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", "..", ".."))

from pyhelpers.setup import chdir
from pyhelpers.setup import get_version
from pyhelpers.setup import get_requirements


with chdir(os.path.abspath(os.path.dirname(__file__))):
    setuptools.setup(
        name="omim-descriptions",
        version=str(get_version()),
        author="Organic Maps",
        author_email="info@organicmaps.app",
        description="This package is a library that provides descriptions "
        "(such as those from Wikipedia) to geographic objects.",
        url="https://github.com/organicmaps",
        package_dir={"descriptions": ""},
        packages=["descriptions"],
        classifiers=[
            "Programming Language :: Python :: 3",
            "License :: OSI Approved :: Apache Software License",
        ],
        python_requires=">=3.6",
        install_requires=get_requirements(),
    )
