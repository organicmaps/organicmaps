#!/usr/bin/env python
import os
import sys

module_dir = os.path.abspath(os.path.dirname(__file__))
sys.path.insert(0, os.path.join(module_dir, "..", ".."))

from data.base import get_version
from data.base import setup

setup(
    __file__,
    "essential",
    [
        "booking_excluded.txt",
        "borders_vs_osm.csv",
        "categories_brands.txt",
        "categories_cuisines.txt",
        "categories.txt",
        "classificator.txt",
        "colors.txt",
        "countries_meta.txt",
        "countries_synonyms.csv",
        "countries.txt",
        "external_resources.txt",
        "fonts_blacklist.txt",
        "fonts_whitelist.txt",
        "hierarchy.txt",
        "local_ads_symbols.txt",
        "mapcss-dynamic.txt",
        "mapcss-mapping.csv",
        "mixed_nodes.txt",
        "mixed_tags.txt",
        "mwm_names_en.txt",
        "old_vs_new.csv",
        "patterns.txt",
        "replaced_tags.txt",
        "skipped_elements.json",
        "synonyms.txt",
        "transit_colors.txt",
        "types.txt",
        "ugc_types.csv",
        "unicode_blocks.txt",
        "visibility.txt",
    ],
    install_requires=["omim-data-files=={}".format(get_version())]
)
