import collections
import datetime
import logging
import logging.config
import os
import shutil
import sys

from . import settings
from .osmtools import build_osmtools
from .status import Status
from ..utils.file import find_executable, is_executable, symlink_force

logger = logging.getLogger("maps_generator")

WORLD_NAME = "World"
WORLD_COASTS_NAME = "WorldCoasts"

WORLDS_NAMES = {WORLD_NAME, WORLD_COASTS_NAME}


def _write_version(out_path, version):
    with open(os.path.join(out_path, settings.VERSION_FILE_NAME), "w") as f:
        f.write(str(version))


def _read_version(version_path):
    with open(version_path) as f:
        line = f.readline().strip()
        try:
            return int(line)
        except ValueError:
            logger.exception(f"Cast '{line}' to int error.")
            return 0


def find_last_build_dir(hint):
    if hint:
        p = os.path.join(settings.MAIN_OUT_PATH, hint)
        return hint if os.path.exists(p) else None
    try:
        paths = [os.path.join(settings.MAIN_OUT_PATH, f)
                 for f in os.listdir(settings.MAIN_OUT_PATH)]
    except FileNotFoundError:
        logger.exception(f"{settings.MAIN_OUT_PATH} not found.")
        return None
    versions = []
    for path in paths:
        version_path = os.path.join(path, settings.VERSION_FILE_NAME)
        if not os.path.isfile(version_path):
            versions.append(0)
        else:
            versions.append(_read_version(version_path))
    pairs = sorted(zip(paths, versions), key=lambda p: p[1], reverse=True)
    return (None if not pairs or pairs[0][1] == 0
            else pairs[0][0].split(os.sep)[-1])


def planet_lock_file():
    return f"{settings.PLANET_O5M}.lock"


def build_lock_file(out_path):
    return f"{os.path.join(out_path, 'lock')}.lock"


class Env:
    def __init__(self, options):
        Env._logging_setup()
        logger.info("Start setup ...")
        for k, v in Env._osm_tools_setup().items():
            setattr(self, k, v)
        for k, v in options.items():
            setattr(self, k, v)

        self.gen_tool = Env._generator_tool_setup()

        setup_options = Env._out_path_setup(self.build_name)
        self.out_path, _, self.mwm_version, self.planet_version = setup_options
        logger.info(f"Out path is {self.out_path}.")

        self.intermediate_path = Env._intermediate_path_setup(self.out_path)
        self.data_path = self.intermediate_path
        self.intermediate_tmp_path = os.path.join(self.intermediate_path, "tmp")
        self._create_if_not_exist(self.intermediate_tmp_path)
        Env._tmp_dir_setup()

        self.mwm_path = os.path.join(self.out_path, str(self.mwm_version))
        self._create_if_not_exist(self.mwm_path)

        self.log_path = os.path.join(self.out_path, "logs")
        self._create_if_not_exist(self.log_path)

        self.temp_borders_path = self._prepare_borders()

        self.draft_path = os.path.join(self.out_path, "draft")
        self._create_if_not_exist(self.draft_path)
        symlink_force(self.temp_borders_path,
                      os.path.join(self.draft_path, "borders"))

        self.osm2ft_path = os.path.join(self.out_path, "osm2ft")
        self._create_if_not_exist(self.osm2ft_path)
        for x in os.listdir(self.osm2ft_path):
            p = os.path.join(self.osm2ft_path, x)
            if os.path.isfile(p) and x.endswith(".mwm.osm2ft"):
                shutil.move(p, os.path.join(self.mwm_path, x))

        self.node_storage = settings.NODE_STORAGE
        self.user_resource_path = settings.USER_RESOURCE_PATH

        self.coastline_path = os.path.join(self.intermediate_path, "coasts")
        self._create_if_not_exist(self.coastline_path)

        self.status_path = os.path.join(self.out_path, "status")
        self._create_if_not_exist(self.status_path)
        self.countries_meta = collections.defaultdict(dict)

        self.main_status_path = os.path.join(self.status_path, "stages.status")
        self.main_status = Status()

        self.coastline_tmp_path = os.path.join(self.coastline_path, "tmp")
        self._create_if_not_exist(self.coastline_tmp_path)
        
        self.srtm_path = settings.SRTM_PATH

        self._subprocess_out = None
        self._subprocess_countries_out = {}

        _write_version(self.out_path, self.planet_version)

        self._skipped_stages = set(self.skip)

        printed_countries = ", ".join(self.countries)
        if len(self.countries) > 50:
            printed_countries = (f"{', '.join(self.countries[:25])}, ..., "
                                 f"{', '.join(self.countries[-25:])}")
        logger.info(f"The following {len(self.countries)} maps will build: "
                    f"{printed_countries}.")
        logger.info("Finish setup")

    def get_mwm_names(self):
        tmp_ext = ".mwm.tmp"
        existing_names = set()
        for f in os.listdir(self.intermediate_tmp_path):
            path = os.path.join(self.intermediate_tmp_path, f)
            if f.endswith(tmp_ext) and os.path.isfile(path):
                name = f.replace(tmp_ext, "")
                if name in self.countries:
                    existing_names.add(name)
        return [c for c in self.countries if c in existing_names]

    def is_accepted_stage(self, stage):
        return stage.__name__ not in self._skipped_stages

    @property
    def descriptions_path(self):
        path = os.path.join(self.intermediate_path, "descriptions")
        self._create_if_not_exist(path)
        return path

    @property
    def packed_polygons_path(self):
        return os.path.join(self.intermediate_path, "packed_polygons.bin")

    @property
    def localads_path(self):
        path = os.path.join(self.out_path, f"localads_{self.mwm_version}")
        self._create_if_not_exist(path)
        return path

    @property
    def stats_path(self):
        path = os.path.join(self.out_path, "stats")
        self._create_if_not_exist(path)
        return path

    @property
    def types_path(self):
        return os.path.join(self.user_resource_path, "types.txt")

    @property
    def external_resources_path(self):
        return os.path.join(self.mwm_path, "external_resources.txt")

    @property
    def id_to_wikidata_path(self):
        return os.path.join(self.intermediate_path, "id_to_wikidata.csv")

    @property
    def wiki_url_path(self):
        return os.path.join(self.intermediate_path, "wiki_urls.txt")

    @property
    def ugc_path(self):
        return os.path.join(self.intermediate_path, "ugc_db.sqlite3")

    @property
    def hotels_path(self):
        return os.path.join(self.intermediate_path, "hotels.csv")

    @property
    def promo_catalog_cities_path(self):
        return os.path.join(self.intermediate_path, "promo_catalog_cities.json")

    @property
    def promo_catalog_countries_path(self):
        return os.path.join(self.intermediate_path,
                            "promo_catalog_countries.json")

    @property
    def popularity_path(self):
        return os.path.join(self.intermediate_path, "popular_places.csv")

    @property
    def subway_path(self):
        return os.path.join(self.intermediate_path,
                            "mapsme_osm_subways.transit.json")

    @property
    def food_paths(self):
        return os.path.join(self.intermediate_path, "ids_food.json")

    @property
    def food_translations_path(self):
        return os.path.join(self.intermediate_path, "translations_food.json")

    @property
    def postcodes_path(self):
        return os.path.join(self.intermediate_path, "postcodes")

    @property
    def cities_boundaries_path(self):
        return os.path.join(self.intermediate_path, "cities_boundaries.bin")

    @property
    def transit_path(self):
        return self.intermediate_path

    @property
    def hierarchy_path(self):
        return os.path.join(self.user_resource_path, "hierarchy.txt")

    @property
    def old_to_new_path(self):
        return os.path.join(self.user_resource_path, "old_vs_new.csv")

    @property
    def borders_to_osm_path(self):
        return os.path.join(self.user_resource_path, "borders_vs_osm.csv")

    @property
    def countries_synonyms_path(self):
        return os.path.join(self.user_resource_path, "countries_synonyms.csv")

    @property
    def counties_txt_path(self):
        return os.path.join(self.mwm_path, "countries.txt")

    def __getitem__(self, item):
        return self.__dict__[item]

    def finish(self):
        self.main_status.finish()

    def finish_mwm(self, mwm_name):
        self.countries_meta[mwm_name]["status"].finish()

    def set_subprocess_out(self, subprocess_out, country=None):
        if country is None:
            self._subprocess_out = subprocess_out
        else:
            self._subprocess_countries_out[country] = subprocess_out

    def get_subprocess_out(self, country=None):
        if country is None:
            return self._subprocess_out
        else:
            return self._subprocess_countries_out[country]

    @staticmethod
    def _logging_setup():
        def exception_handler(type, value, tb):
            logger.exception(f"Uncaught exception: {str(value)}",
                             exc_info=(type, value, tb))

        logging.config.dictConfig(settings.LOGGING)
        sys.excepthook = exception_handler

    @staticmethod
    def _generator_tool_setup():
        logger.info("Check generator tool ...")
        gen_tool_path = shutil.which(settings.GEN_TOOL)
        if gen_tool_path is None:
            logger.info(f"Find generator tool in {settings.BUILD_PATH} ...")
            gen_tool_path = find_executable(settings.BUILD_PATH,
                                            settings.GEN_TOOL)
        logger.info(f"Generator found - {gen_tool_path}")
        return gen_tool_path

    @staticmethod
    def _osm_tools_setup():
        path = settings.OSM_TOOLS_PATH
        osm_tool_names = [
            settings.OSM_TOOL_CONVERT, settings.OSM_TOOL_UPDATE,
            settings.OSM_TOOL_FILTER
        ]

        logger.info("Check osm tools ...")
        if not Env._create_if_not_exist(path):
            tmp_paths = [os.path.join(path, t) for t in osm_tool_names]
            if all([is_executable(t) for t in tmp_paths]):
                osm_tool_paths = dict(zip(osm_tool_names, tmp_paths))
                logger.info(
                    f"Osm tools found - {', '.join(osm_tool_paths.values())}")
                return osm_tool_paths

        tmp_paths = [shutil.which(t) for t in osm_tool_names]
        if all(tmp_paths):
            osm_tool_paths = dict(zip(osm_tool_names, tmp_paths))
            logger.info(
                f"Osm tools found - {', '.join(osm_tool_paths.values())}")
            return osm_tool_paths

        logger.info("Build osm tools ...")
        return build_osmtools(settings.OSM_TOOLS_SRC_PATH)

    @staticmethod
    def _out_path_setup(build_name):
        dt = datetime.datetime.now()
        version_format = "%Y_%m_%d__%H_%M_%S"
        if build_name:
            dt = datetime.datetime.strptime(build_name, version_format)

        s = dt.strftime(version_format)
        mwm_version = dt.strftime("%y%m%d")
        planet_version = int(dt.strftime("%s"))

        out_path = os.path.join(settings.MAIN_OUT_PATH, s)
        Env._create_if_not_exist(settings.MAIN_OUT_PATH)
        Env._create_if_not_exist(out_path)
        return out_path, s, mwm_version, planet_version

    @staticmethod
    def _intermediate_path_setup(out_path):
        intermediate_path = os.path.join(out_path, "intermediate_data")
        Env._create_if_not_exist(intermediate_path)
        return intermediate_path

    @staticmethod
    def _tmp_dir_setup():
        Env._create_if_not_exist(settings.TMPDIR)
        os.environ["TMPDIR"] = settings.TMPDIR

    @staticmethod
    def _create_if_not_exist(path):
        try:
            os.mkdir(path)
            logger.info(f"Create {path} ...")
            return True
        except FileExistsError:
            return False

    def _prepare_borders(self):
        borders = "borders"
        temp_borders = os.path.join(self.intermediate_path, borders)
        Env._create_if_not_exist(temp_borders)
        borders = os.path.join(settings.USER_RESOURCE_PATH, borders)
        for x in self.countries:
            if x in WORLDS_NAMES:
                continue
            shutil.copy2(f"{os.path.join(borders, x)}.poly", temp_borders)
        return temp_borders
