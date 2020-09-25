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
        name="omim-airmaps",
        version=str(get_version()),
        author="My.com B.V. (Mail.Ru Group)",
        author_email="dev@maps.me",
        description="This package contains tools for generating maps with Apache Airflow.",
        url="https://github.com/mapsme",
        package_dir={"airmaps": ""},
        package_data={"": ["var/**/*"]},
        packages=[
            "airmaps",
            "airmaps.dags",
            "airmaps.instruments",
        ],
        classifiers=[
            "Programming Language :: Python :: 3",
            "License :: OSI Approved :: Apache Software License",
        ],
        python_requires=">=3.6",
        install_requires=get_requirements(),
    )
