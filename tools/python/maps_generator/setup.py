#!/usr/bin/env python3
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
        name="omim-maps_generator",
        version=str(get_version()),
        author="CoMaps",
        author_email="info@comaps.app",
        description="This package contains tools for maps generation.",
        url="https://codeberg.org/comaps",
        package_dir={"maps_generator": ""},
        package_data={"": ["var/**/*"]},
        packages=[
            "maps_generator",
            "maps_generator.generator",
            "maps_generator.utils",
            "maps_generator.checks"
        ],
        classifiers=[
            "Programming Language :: Python :: 3",
            "License :: OSI Approved :: Apache Software License",
        ],
        python_requires=">=3.6",
        install_requires=get_requirements(),
    )
