""""
This file contains possible stages that maps_generator can run.
Some algorithms suppose a maps genration processes looks like:
    stage1, ..., stage_mwm[country_stage_1, ..., country_stage_M], ..., stageN
Only stage_mwm can contain country_
"""
import datetime
import json
import logging
import multiprocessing
import os
import shutil
import tarfile
from collections import defaultdict
from functools import partial
from multiprocessing.pool import ThreadPool
from typing import Type

from descriptions.descriptions_downloader import check_and_get_checker
from descriptions.descriptions_downloader import download_from_wikidata_tags
from descriptions.descriptions_downloader import download_from_wikipedia_tags
from post_generation.hierarchy_to_countries import hierarchy_to_countries
from post_generation.inject_promo_ids import inject_promo_ids
from post_generation.localads_mwm_to_csv import create_csv
from . import coastline
from . import settings
from . import steps
from .env import Env
from .env import PathProvider
from .env import WORLDS_NAMES
from .env import WORLD_COASTS_NAME
from .env import WORLD_NAME
from .exceptions import BadExitStatusError
from .gen_tool import run_gen_tool
from .stages import Stage
from .stages import build_lock
from .stages import country_stage
from .stages import get_stage_name
from .stages import helper_stage
from .stages import mwm_stage
from .stages import outer_stage
from .stages import planet_lock
from .stages import production_only
from .statistics import get_stages_info
from .statistics import make_stats
from ..utils.file import download_files
from ..utils.file import is_verified

logger = logging.getLogger("maps_generator")


def is_skipped(env: Env, stage: Type[Stage]) -> bool:
    return env.is_skipped_stage_name(get_stage_name(stage))


@outer_stage
class StageDownloadExternal(Stage):
    def apply(self, env: Env):
        download_files(
            {settings.SUBWAY_URL: env.paths.subway_path,}
        )


@outer_stage
@production_only
class StageDownloadProductionExternal(Stage):
    def apply(self, env: Env):
        download_files(
            {
                settings.UGC_URL: env.paths.ugc_path,
                settings.HOTELS_URL: env.paths.hotels_path,
                settings.PROMO_CATALOG_CITIES_URL: env.paths.promo_catalog_cities_path,
                settings.PROMO_CATALOG_COUNTRIES_URL: env.paths.promo_catalog_countries_path,
                settings.POPULARITY_URL: env.paths.popularity_path,
                settings.FOOD_URL: env.paths.food_paths,
                settings.FOOD_TRANSLATIONS_URL: env.paths.food_translations_path,
                settings.UK_POSTCODES_URL: env.paths.uk_postcodes_path,
                settings.US_POSTCODES_URL: env.paths.us_postcodes_path,
            }
        )


@outer_stage
@planet_lock
class StageDownloadAndConvertPlanet(Stage):
    def apply(self, env: Env, **kwargs):
        force_download = not is_skipped(env, StageUpdatePlanet)
        if force_download or not is_verified(settings.PLANET_O5M):
            steps.step_download_and_convert_planet(
                env, force_download=force_download, **kwargs
            )


@outer_stage
@planet_lock
class StageUpdatePlanet(Stage):
    def apply(self, env: Env, **kwargs):
        if not settings.DEBUG:
            steps.step_update_planet(env, **kwargs)


@outer_stage
@build_lock
class StageCoastline(Stage):
    def apply(self, env: Env, use_old_if_fail=True):
        coasts_geom = "WorldCoasts.geom"
        coasts_rawgeom = "WorldCoasts.rawgeom"
        try:
            coastline.make_coastline(env)
        except BadExitStatusError as e:
            if not use_old_if_fail:
                raise e

            logger.info("Build costs failed. Try to download the costs...")
            download_files(
                {
                    settings.PLANET_COASTS_GEOM_URL: os.path.join(
                        env.paths.coastline_path, coasts_geom
                    ),
                    settings.PLANET_COASTS_RAWGEOM_URL: os.path.join(
                        env.paths.coastline_path, coasts_rawgeom
                    ),
                }
            )

        for f in [coasts_geom, coasts_rawgeom]:
            path = os.path.join(env.paths.coastline_path, f)
            shutil.copy2(path, env.paths.intermediate_data_path)


@outer_stage
@build_lock
class StagePreprocess(Stage):
    def apply(self, env: Env, **kwargs):
        steps.step_preprocess(env, **kwargs)


@outer_stage
@build_lock
class StageFeatures(Stage):
    def apply(self, env: Env):
        extra = {}
        if is_skipped(env, StageDescriptions):
            extra["idToWikidata"] = env.paths.id_to_wikidata_path
        if is_skipped(env, StageDownloadProductionExternal):
            extra["booking_data"] = env.paths.hotels_path
            extra["promo_catalog_cities"] = env.paths.promo_catalog_cities_path
            extra["popular_places_data"] = env.paths.popularity_path
            extra["brands_data"] = env.paths.food_paths
            extra["brands_translations_data"] = env.paths.food_translations_path
        if is_skipped(env, StageCoastline):
            extra["emit_coasts"] = True
        if is_skipped(env, StageIsolinesInfo):
            extra["isolines_path"] = PathProvider.isolines_path()

        steps.step_features(env, **extra)
        if os.path.exists(env.paths.packed_polygons_path):
            shutil.copy2(env.paths.packed_polygons_path, env.paths.mwm_path)


@outer_stage
@build_lock
@production_only
@helper_stage
class StageDownloadDescriptions(Stage):
    def apply(self, env: Env):
        if not is_skipped(env, StageDescriptions):
            return

        run_gen_tool(
            env.gen_tool,
            out=env.get_subprocess_out(),
            err=env.get_subprocess_out(),
            intermediate_data_path=env.paths.intermediate_data_path,
            user_resource_path=env.paths.user_resource_path,
            dump_wikipedia_urls=env.paths.wiki_url_path,
            idToWikidata=env.paths.id_to_wikidata_path,
        )

        langs = ("en", "ru", "es", "fr", "de")
        checker = check_and_get_checker(env.paths.popularity_path)
        download_from_wikipedia_tags(
            env.paths.wiki_url_path, env.paths.descriptions_path, langs, checker
        )
        download_from_wikidata_tags(
            env.paths.id_to_wikidata_path, env.paths.descriptions_path, langs, checker
        )


@outer_stage
@build_lock
@mwm_stage
class StageMwm(Stage):
    def apply(self, env: Env):
        def build(country):
            StageIndex(country=country)(env)
            StageUgc(country=country)(env)
            StagePopularity(country=country)(env)
            StageSrtm(country=country)(env)
            StageDescriptions(country=country)(env)
            StageRouting(country=country)(env)
            StageRoutingTransit(country=country)(env)
            env.finish_mwm(country)

        def build_world(country):
            StageIndex(country=country)(env)
            StageCitiesIdsWorld(country=country)(env)
            env.finish_mwm(country)

        def build_world_coasts(country):
            StageIndex(country=country)(env)
            env.finish_mwm(country)

        specific = {WORLD_NAME: build_world, WORLD_COASTS_NAME: build_world_coasts}
        names = env.get_tmp_mwm_names()
        with ThreadPool() as pool:
            pool.map(
                lambda c: specific[c](c) if c in specific else build(c),
                names,
                chunksize=1,
            )


@country_stage
@build_lock
class StageIndex(Stage):
    def apply(self, env: Env, country, **kwargs):
        if country == WORLD_NAME:
            steps.step_index_world(env, country, **kwargs)
        elif country == WORLD_COASTS_NAME:
            steps.step_coastline_index(env, country, **kwargs)
        else:
            extra = {}
            if is_skipped(env, StageDownloadProductionExternal):
                extra["uk_postcodes_dataset"] = env.paths.uk_postcodes_path
                extra["us_postcodes_dataset"] = env.paths.us_postcodes_path
            steps.step_index(env, country, **kwargs, **extra)


@country_stage
@build_lock
@production_only
class StageCitiesIdsWorld(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_cities_ids_world(env, country, **kwargs)


@country_stage
@build_lock
@production_only
class StageUgc(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_ugc(env, country, **kwargs)


@country_stage
@build_lock
@production_only
class StagePopularity(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_popularity(env, country, **kwargs)


@country_stage
@build_lock
@production_only
class StageSrtm(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_srtm(env, country, **kwargs)


@country_stage
@build_lock
@production_only
class StageIsolinesInfo(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_isolines_info(env, country, **kwargs)


@country_stage
@build_lock
@production_only
class StageDescriptions(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_description(env, country, **kwargs)


@country_stage
@build_lock
class StageRouting(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_routing(env, country, **kwargs)


@country_stage
@build_lock
class StageRoutingTransit(Stage):
    def apply(self, env: Env, country, **kwargs):
        steps.step_routing_transit(env, country, **kwargs)


@outer_stage
@build_lock
class StageCountriesTxt(Stage):
    def apply(self, env: Env):
        countries = hierarchy_to_countries(
            env.paths.old_to_new_path,
            env.paths.borders_to_osm_path,
            env.paths.countries_synonyms_path,
            env.paths.hierarchy_path,
            env.paths.mwm_path,
            env.paths.mwm_version,
        )
        if is_skipped(env, StageDownloadProductionExternal):
            countries_json = json.loads(countries)
            inject_promo_ids(
                countries_json,
                env.paths.promo_catalog_cities_path,
                env.paths.promo_catalog_countries_path,
                env.paths.mwm_path,
                env.paths.types_path,
                env.paths.mwm_path,
            )
            countries = json.dumps(countries_json, ensure_ascii=True, indent=1)
        with open(env.paths.counties_txt_path, "w") as f:
            f.write(countries)


@outer_stage
@build_lock
class StageExternalResources(Stage):
    def apply(self, env: Env):
        black_list = {"00_roboto_regular.ttf"}
        resources = [
            os.path.join(env.paths.user_resource_path, file)
            for file in os.listdir(env.paths.user_resource_path)
            if file.endswith(".ttf") and file not in black_list
        ]
        for ttf_file in resources:
            shutil.copy2(ttf_file, env.paths.mwm_path)

        for file in os.listdir(env.paths.mwm_path):
            if file.startswith(WORLD_NAME) and file.endswith(".mwm"):
                resources.append(os.path.join(env.paths.mwm_path, file))

        resources.sort()
        with open(env.paths.external_resources_path, "w") as f:
            for resource in resources:
                fd = os.open(resource, os.O_RDONLY)
                f.write(f"{os.path.basename(resource)} {os.fstat(fd).st_size}\n")


@outer_stage
@build_lock
@production_only
class StageLocalads(Stage):
    def apply(self, env: Env):
        create_csv(
            env.paths.localads_path,
            env.paths.mwm_path,
            env.paths.mwm_path,
            env.paths.types_path,
            env.mwm_version,
            multiprocessing.cpu_count(),
        )
        with tarfile.open(f"{env.paths.localads_path}.tar.gz", "w:gz") as tar:
            for filename in os.listdir(env.paths.localads_path):
                tar.add(
                    os.path.join(env.paths.localads_path, filename), arcname=filename
                )


@outer_stage
@build_lock
class StageStatistics(Stage):
    def apply(self, env: Env):
        result = defaultdict(lambda: defaultdict(dict))

        def stage_mwm_statistics(env: Env, country, **kwargs):
            stats_tmp = os.path.join(env.paths.draft_path, f"{country}.stat")
            with open(os.devnull, "w") as dev_null:
                with open(stats_tmp, "w") as f:
                    steps.run_gen_tool_with_recovery_country(
                        env,
                        env.gen_tool,
                        out=f,
                        err=dev_null,
                        data_path=env.paths.mwm_path,
                        user_resource_path=env.paths.user_resource_path,
                        type_statistics=True,
                        output=country,
                        **kwargs,
                    )
            result["countries"][country]["types"] = make_stats(
                settings.STATS_TYPES_CONFIG, stats_tmp
            )

        names = env.get_tmp_mwm_names()
        countries = filter(lambda x: x not in WORLDS_NAMES, names)
        with ThreadPool() as pool:
            pool.map(partial(stage_mwm_statistics, env), countries)

        steps_info = get_stages_info(env.paths.log_path, {"statistics"})
        result["steps"] = steps_info["steps"]
        for c in steps_info["countries"]:
            result["countries"][c]["steps"] = steps_info["countries"][c]

        def default(o):
            if isinstance(o, datetime.timedelta):
                return str(o)

        with open(os.path.join(env.paths.stats_path, "stats.json"), "w") as f:
            json.dump(
                result, f, ensure_ascii=False, sort_keys=True, indent=2, default=default
            )


@outer_stage
@build_lock
class StageCleanup(Stage):
    def apply(self, env: Env):
        logger.info(
            f"osm2ft files will be moved from {env.paths.build_path} "
            f"to {env.paths.osm2ft_path}."
        )
        for x in os.listdir(env.paths.mwm_path):
            p = os.path.join(env.paths.mwm_path, x)
            if os.path.isfile(p) and x.endswith(".mwm.osm2ft"):
                shutil.move(p, os.path.join(env.paths.osm2ft_path, x))

        logger.info(f"{env.paths.draft_path} will be removed.")
        shutil.rmtree(env.paths.draft_path)
