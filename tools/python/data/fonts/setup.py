#!/usr/bin/env python
import os
import sys

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", ".."))

from data.base import get_version
from data.base import setup

setup(
    __file__,
    "fonts",
    [
        "00_NotoNaskhArabic-Regular.ttf",
        "00_NotoSansThai-Regular.ttf",
        "01_dejavusans.ttf",
        "02_droidsans-fallback.ttf",
        "03_jomolhari-id-a3d.ttf",
        "04_padauk.ttf",
        "05_khmeros.ttf",
        "06_code2000.ttf",
        "07_roboto_medium.ttf",
    ],
    install_requires=["omim-data-files=={}".format(get_version())]
)
