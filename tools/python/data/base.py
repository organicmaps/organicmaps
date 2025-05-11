import os
import sys
from collections import defaultdict

import setuptools

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", "..", ".."))

from pyhelpers.setup import chdir
from pyhelpers.setup import get_version


DATA_PATH = os.path.abspath(
    os.path.join(os.path.dirname(__file__), "..", "..", "..", "data")
)


def get_files_from_dir(abs_root_path, b, data_files):
    for root, dirs, files in os.walk(abs_root_path):
        data_files[b].extend(os.path.join(root, f) for f in files)
        for d in dirs:
            get_files_from_dir(
                os.path.join(abs_root_path, d), os.path.join(b, d), data_files
            )


def get_data_files(relative_data_paths):
    data_files = defaultdict(lambda: [])
    for p in relative_data_paths:
        path = os.path.join(DATA_PATH, p)
        b = os.path.join("omim-data", path.replace(DATA_PATH + os.path.sep, ""))
        if os.path.isdir(path):
            get_files_from_dir(path, b, data_files)
        else:
            b = os.path.dirname(b)
            data_files[b].append(path)
    return data_files.items()


def setup(
    source_file,
    suffix,
    relative_data_paths,
    packages=None,
    package_dir=None,
    install_requires=None,
    cmdclass=None,
    supported_pythons=("2", "2.7", "3", "3.5", "3.6", "3.7", "3.8", "3.9"),
):
    with chdir(os.path.abspath(os.path.dirname(source_file))):
        setuptools.setup(
            name="omim-data-{}".format(suffix),
            version=str(get_version()),
            author="CoMaps",
            author_email="info@comaps.app",
            description="This package contains {} data files.".format(suffix),
            url="https://codeberg.org/comaps",
            packages=[] if packages is None else packages,
            package_dir={} if package_dir is None else package_dir,
            cmdclass={} if cmdclass is None else cmdclass,
            classifiers=["License :: OSI Approved :: Apache Software License",]
            + [
                "Programming Language :: Python :: {}".format(supported_python)
                for supported_python in supported_pythons
            ],
            install_requires=install_requires or [],
            data_files=get_data_files(relative_data_paths),
        )
