import argparse
import multiprocessing
import os
import sys
from configparser import ConfigParser, ExtendedInterpolation
from pathlib import Path

from ..utils.system import total_virtual_memory

SETTINGS_PATH = os.path.dirname(os.path.join(os.path.realpath(__file__)))

parser = argparse.ArgumentParser(add_help=False)
opt_config = "--config"
parser.add_argument(opt_config, type=str, default="", help="Path to config")


def get_config_path():
    config_path = os.path.join(SETTINGS_PATH, "../var/etc/map_generator.ini")
    argv = sys.argv
    indexes = (-1, -1)
    for i, opt in enumerate(argv):
        if opt.startswith(f"{opt_config}="):
            indexes = (i, i + 1)
        if opt == opt_config:
            indexes = (i, i + 2)

    if indexes[1] > len(argv):
        return config_path

    args = argv[indexes[0]: indexes[1]]
    return parser.parse_args(args).config if args else config_path


DEBUG = True
HOME_PATH = str(Path.home())
WORK_PATH = HOME_PATH
TMPDIR = os.path.join(HOME_PATH, "tmp")
MAIN_OUT_PATH = os.path.join(WORK_PATH, "generation")

VERSION_FILE_NAME = "version.txt"

# External resources
DEFAULT_PLANET_URL = "https://planet.openstreetmap.org/pbf/planet-latest.osm.pbf"
DEFAULT_PLANET_MD5_URL = DEFAULT_PLANET_URL + ".md5"
PLANET_COASTS_URL = ""
UGC_URL = ""
HOTELS_URL = ""
PROMO_CATALOG_CITIES_URL = ""
PROMO_CATALOG_COUNTRIES_URL = ""
POPULARITY_URL= ""
SUBWAY_URL = ""
FOOD_URL = ""
FOOD_TRANSLATIONS_URL = ""
POSTCODES_URL = ""
SRTM_PATH = ""

STATS_TYPES_CONFIG = ""

PLANET = "planet"

GEN_TOOL = "generator_tool"

BUILD_PATH = os.path.join(WORK_PATH, "omim-build-release")
OMIM_PATH = os.path.join(WORK_PATH, "omim")

# generator_tool
NODE_STORAGE = "mem" if total_virtual_memory() / 10 ** 9 >= 64 else "map"
USER_RESOURCE_PATH = os.path.join(OMIM_PATH, "data")

# osm tools
OSM_TOOL_CONVERT = "osmconvert"
OSM_TOOL_FILTER = "osmfilter"
OSM_TOOL_UPDATE = "osmupdate"
OSM_TOOLS_SRC_PATH = os.path.join(OMIM_PATH, "tools", "osmctools")
OSM_TOOLS_PATH = os.path.join(WORK_PATH, "osmctools")
OSM_TOOLS_CC = "cc"
OSM_TOOLS_CC_FLAGS = ["-O3", ]

# system
CPU_COUNT = multiprocessing.cpu_count()

# Try to read a config and to overload default settings
config = ConfigParser(interpolation=ExtendedInterpolation())
config.read([get_config_path()])


def _get_opt(config, s, v, default=None):
    return config.get(s, v) if config.has_option(s, v) else default


def _get_opt_path(config, s, v, default=""):
    return os.path.expanduser(_get_opt(config, s, v, default))


_DEBUG = _get_opt(config, "Main", "DEBUG")
DEBUG = DEBUG if _DEBUG is None else int(_DEBUG)
MAIN_OUT_PATH = _get_opt_path(config, "Main", "MAIN_OUT_PATH", MAIN_OUT_PATH)

# logging
LOG_FILE_PATH = os.path.join(MAIN_OUT_PATH, "generation.log")

TMPDIR = _get_opt_path(config, "Main", "TMPDIR", TMPDIR)

BUILD_PATH = _get_opt_path(config, "Developer", "BUILD_PATH", BUILD_PATH)
OMIM_PATH = _get_opt_path(config, "Developer", "OMIM_PATH", OMIM_PATH)

USER_RESOURCE_PATH = _get_opt_path(config, "Generator tool",
                                   "USER_RESOURCE_PATH", USER_RESOURCE_PATH)

NODE_STORAGE = _get_opt(config, "Generator tool", "NODE_STORAGE", NODE_STORAGE)

OSM_TOOLS_SRC_PATH = _get_opt_path(config, "Osm tools", "OSM_TOOLS_SRC_PATH", OSM_TOOLS_SRC_PATH)
OSM_TOOLS_PATH = _get_opt_path(config, "Osm tools", "OSM_TOOLS_PATH", OSM_TOOLS_PATH)

LOG_FILE_PATH = _get_opt_path(config, "Logging", "MAIN_LOG", LOG_FILE_PATH)
os.makedirs(os.path.dirname(os.path.abspath(LOG_FILE_PATH)), exist_ok=True)

PLANET_URL = _get_opt_path(config, "External", "PLANET_URL", DEFAULT_PLANET_URL)
PLANET_MD5_URL = _get_opt_path(config, "External", "PLANET_MD5_URL", DEFAULT_PLANET_MD5_URL)
PLANET_COASTS_URL = _get_opt_path(config, "External", "PLANET_COASTS_URL", PLANET_COASTS_URL)
UGC_URL = _get_opt_path(config, "External", "UGC_URL", UGC_URL)
HOTELS_URL = _get_opt_path(config, "External", "HOTELS_URL", HOTELS_URL)
PROMO_CATALOG_CITIES_URL = _get_opt_path(config, "External", "PROMO_CATALOG_CITIES_URL", PROMO_CATALOG_CITIES_URL)
PROMO_CATALOG_COUNTRIES_URL = _get_opt_path(config, "External", "PROMO_CATALOG_COUNTRIES_URL", PROMO_CATALOG_COUNTRIES_URL)
POPULARITY_URL = _get_opt_path(config, "External", "POPULARITY_URL", POPULARITY_URL)
SUBWAY_URL = _get_opt(config, "External", "SUBWAY_URL", SUBWAY_URL)
FOOD_URL = _get_opt(config, "External", "FOOD_URL", FOOD_URL)
POSTCODES_URL = _get_opt(config, "External", "POSTCODES_URL", POSTCODES_URL)
FOOD_TRANSLATIONS_URL = _get_opt(config, "External", "FOOD_TRANSLATIONS_URL", 
                                 FOOD_TRANSLATIONS_URL)
SRTM_PATH = _get_opt_path(config, "External", "SRTM_PATH", SRTM_PATH)

STATS_TYPES_CONFIG = _get_opt_path(config, "Stats", "STATS_TYPES_CONFIG",
                                   STATS_TYPES_CONFIG)

PLANET_O5M = os.path.join(MAIN_OUT_PATH, PLANET + ".o5m")
PLANET_PBF = os.path.join(MAIN_OUT_PATH, PLANET + ".osm.pbf")
PLANET_COASTS_GEOM_URL = os.path.join(PLANET_COASTS_URL, "latest_coasts.geom")
PLANET_COASTS_RAWGEOM_URL = os.path.join(PLANET_COASTS_URL, "latest_coasts.rawgeom")

if DEBUG:
    PLANET_URL = "http://osmz.ru/mwm/islands/islands.o5m"
    PLANET_MD5_URL = "https://cloud.mail.ru/public/5v2F/f7cSaEXBC"

# for lib logging
LOGGING = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {
        "standard": {
            "format": "[%(asctime)s] %(levelname)s %(module)s %(message)s"
        },
    },
    "handlers": {
        "stdout": {
            "level": "INFO",
            "class": "logging.StreamHandler",
            "formatter": "standard"
        },
        "file": {
            "level": "DEBUG",
            "class": "logging.handlers.WatchedFileHandler",
            "formatter": "standard",
            "filename": LOG_FILE_PATH
        }
    },
    "loggers": {
        "maps_generator": {
            "handlers": ["stdout", "file"],
            "level": "DEBUG",
            "propagate": True
        }
    }
}
