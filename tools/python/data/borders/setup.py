#!/usr/bin/env python
import os
import sys
import tarfile
from distutils import log
from distutils.command.build import build
from distutils.command.clean import clean

from six import BytesIO

try:
    import lzma
except ImportError:
    from backports import lzma


module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", ".."))

from data.base import DATA_PATH
from data.base import chdir
from data.base import get_version
from data.base import setup


TAR_LZMA_PATH = os.path.join(DATA_PATH, "borders.tar.xz")


class BuildCmd(build, object):
    def run(self):
        log.info("Creating {}".format(TAR_LZMA_PATH))
        tar_stream = BytesIO()
        borders_path = os.path.join(DATA_PATH, "borders")
        with chdir(borders_path):
            with tarfile.open(fileobj=tar_stream, mode="w") as tar:
                for f in os.listdir(borders_path):
                    tar.add(f)

        tar_stream.seek(0)
        with lzma.open(TAR_LZMA_PATH, mode="w") as f:
            f.write(tar_stream.read())

        super(BuildCmd, self).run()


class CleanCmd(clean, object):
    def run(self):
        if os.path.exists(TAR_LZMA_PATH):
            log.info("Removing {}".format(TAR_LZMA_PATH))
            os.remove(TAR_LZMA_PATH)

        super(CleanCmd, self).run()


setup(
    __file__,
    "borders",
    ["borders.tar.xz", "packed_polygons.bin"],
    package_dir={"borders": ""},
    packages=["borders"],
    cmdclass={"build": BuildCmd, "clean": CleanCmd},
    install_requires=["omim-data-files=={}".format(get_version())]
)
