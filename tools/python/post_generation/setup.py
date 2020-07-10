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
        name="omim-post_generation",
        version=str(get_version()),
        author="My.com B.V. (Mail.Ru Group)",
        author_email="dev@maps.me",
        description="This package is a library for post-processing the generated maps.",
        url="https://github.com/mapsme",
        package_dir={"post_generation": ""},
        packages=["post_generation"],
        classifiers=[
            "Programming Language :: Python :: 3",
            "License :: OSI Approved :: Apache Software License",
        ],
        python_requires=">=3.6",
        install_requires=get_requirements(),
    )
