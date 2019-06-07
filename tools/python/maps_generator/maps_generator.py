import datetime
import json
import logging
import multiprocessing
import os
import shutil
from collections import defaultdict
from functools import partial
from multiprocessing.pool import ThreadPool

from descriptions.descriptions_downloader import (check_and_get_checker,
                                                  download_from_wikipedia_tags,
                                                  download_from_wikidata_tags)
from filelock import FileLock
from post_generation.hierarchy_to_countries import hierarchy_to_countries
from post_generation.localads_mwm_to_csv import create_csv

from .generator import basic_stages
from .generator import coastline
from .generator import maps_stages
from .generator import settings
from .generator.decorators import stage, country_stage, country_stage_log
from .generator.env import (planet_lock_file, build_lock_file,
                            WORLD_COASTS_NAME, WORLD_NAME, WORLDS_NAMES)
from .generator.exceptions import (ContinueError, BadExitStatusError,
                                   wait_and_raise_if_fail)
from .generator.gen_tool import run_gen_tool
from .generator.statistics import make_stats, get_stages_info
from .utils.file import is_verified, download_file, make_tarfile

logger = logging.getLogger("maps_generator")


def download_external(url_to_path: dict):
    ps = [download_file(k, v) for k, v in url_to_path.items()]
    for p in ps:
        wait_and_raise_if_fail(p)


@stage
def stage_download_and_convert_planet(env, **kwargs):
    if not is_verified(settings.PLANET_O5M):
        basic_stages.stage_download_and_convert_planet(env, **kwargs)


@stage
def stage_update_planet(env, **kwargs):
    if not settings.DEBUG:
        basic_stages.stage_update_planet(env, **kwargs)


@stage
def stage_download_external(env, **kwargs):
    download_external({
        settings.SUBWAY_URL: env.subway_path,
    })


@stage
def stage_download_production_external(env, **kwargs):
    download_external({
        settings.UGC_URL: env.ugc_path,
        settings.HOTELS_URL: env.hotels_path,
        settings.PROMO_CATALOG_CITIES_URL: env.promo_catalog_cities_path,
        settings.POPULARITY_URL: env.popularity_path,
        settings.FOOD_URL: env.food_paths,
        settings.FOOD_TRANSLATIONS_URL: env.food_translations_path,
    })


@stage
def stage_preprocess(env, **kwargs):
    basic_stages.stage_preprocess(env, **kwargs)


@stage
def stage_features(env):
    extra = {}
    if env.is_accepted_stage(stage_descriptions.__name__):
        extra["idToWikidata"] = env.id_to_wikidata_path
    if env.is_accepted_stage(stage_download_production_external.__name__):
        extra["booking_data"] = env.hotels_path
        extra["promo_catalog_cities"] = env.promo_catalog_cities_path
        extra["popular_places_data"] = env.popularity_path
        extra["brands_data"] = env.food_paths
        extra["brands_translations_data"] = env.food_translations_path
    if not env.production:
        extra["no_ads"] = True
    if any(x not in WORLDS_NAMES for x in env.countries):
        extra["split_by_polygons"] = True
        extra["generate_packed_borders"] = True
    if any(x == WORLD_NAME for x in env.countries):
        extra["generate_world"] = True

    run_gen_tool(
        env.gen_tool,
        out=env.get_subprocess_out(),
        err=env.get_subprocess_out(),
        data_path=env.data_path,
        intermediate_data_path=env.intermediate_path,
        osm_file_type="o5m",
        osm_file_name=settings.PLANET_O5M,
        node_storage=env.node_storage,
        user_resource_path=env.user_resource_path,
        dump_cities_boundaries=True,
        cities_boundaries_data=env.cities_boundaries_path,
        generate_features=True,
        emit_coasts=True,
        **extra
    )

    if os.path.exists(env.packed_polygons_path):
        shutil.copy2(env.packed_polygons_path, env.mwm_path)


@stage
def stage_coastline(env):
    coasts_geom = "WorldCoasts.geom"
    coasts_rawgeom = "WorldCoasts.rawgeom"
    try:
        coastline.make_coastline(env)
    except BadExitStatusError:
        logger.info("Build costs failed. Try to download the costs...")
        download_external({
            settings.PLANET_COASTS_GEOM_URL:
                os.path.join(env.coastline_path, coasts_geom),
            settings.PLANET_COASTS_RAWGEOM_URL:
                os.path.join(env.coastline_path, coasts_rawgeom)
        })

    for f in [coasts_geom, coasts_rawgeom]:
        path = os.path.join(env.coastline_path, f)
        shutil.copy2(path, env.intermediate_path)


@country_stage
def stage_index(env, country, **kwargs):
    if country == WORLD_NAME:
        maps_stages.stage_index_world(env, country, **kwargs)
    elif country == WORLD_COASTS_NAME:
        maps_stages.stage_coastline_index(env, country, **kwargs)
    else:
        maps_stages.stage_index(env, country, **kwargs)


@country_stage
def stage_ugc(env, country, **kwargs):
    maps_stages.stage_ugc(env, country, **kwargs)


@country_stage
def stage_popularity(env, country, **kwargs):
    maps_stages.stage_popularity(env, country, **kwargs)


@country_stage
def stage_routing(env, country, **kwargs):
    maps_stages.stage_routing(env, country, **kwargs)


@country_stage
def stage_routing_transit(env, country, **kwargs):
    maps_stages.stage_routing_transit(env, country, **kwargs)


@stage
def stage_mwm(env):
    def build(country):
        stage_index(env, country)
        stage_ugc(env, country)
        stage_popularity(env, country)
        stage_routing(env, country)
        stage_routing_transit(env, country)
        env.finish_mwm(country)

    def build_world(country):
        stage_index(env, country)
        env.finish_mwm(country)

    def build_world_coasts(country):
        stage_index(env, country)
        env.finish_mwm(country)

    specific = {
        WORLD_NAME: build_world,
        WORLD_COASTS_NAME: build_world_coasts
    }

    mwms = env.get_mwm_names()
    with ThreadPool() as pool:
        pool.map(lambda c: specific[c](c) if c in specific else build(c), mwms)


@stage
def stage_descriptions(env):
    run_gen_tool(env.gen_tool,
                 out=env.get_subprocess_out(),
                 err=env.get_subprocess_out(),
                 intermediate_data_path=env.intermediate_path,
                 user_resource_path=env.user_resource_path,
                 dump_wikipedia_urls=env.wiki_url_path,
                 idToWikidata=env.id_to_wikidata_path)

    langs = ("en", "ru", "es")
    checker = check_and_get_checker(env.popularity_path)
    download_from_wikipedia_tags(env.wiki_url_path, env.descriptions_path,
                                 langs, checker)
    download_from_wikidata_tags(env.id_to_wikidata_path, env.descriptions_path,
                                langs, checker)

    @country_stage_log
    def stage_write_descriptions(env, country, **kwargs):
        maps_stages.run_gen_tool_with_recovery_country(
            env,
            env.gen_tool,
            out=env.get_subprocess_out(country),
            err=env.get_subprocess_out(country),
            data_path=env.mwm_path,
            user_resource_path=env.user_resource_path,
            wikipedia_pages=env.descriptions_path,
            idToWikidata=env.id_to_wikidata_path,
            output=country,
            **kwargs
        )

    mwms = env.get_mwm_names()
    countries = filter(lambda x: x not in WORLDS_NAMES, mwms)
    with ThreadPool() as pool:
        pool.map(partial(stage_write_descriptions, env), countries)


@stage
def stage_countries_txt(env):
    countries = hierarchy_to_countries(env.old_to_new_path,
                                       env.borders_to_osm_path,
                                       env.countries_synonyms_path,
                                       env.hierarchy_path, env.mwm_path,
                                       env.mwm_version)
    with open(env.counties_txt_path, "w") as f:
        f.write(countries)


@stage
def stage_external_resources(env):
    resources = [os.path.join(env.user_resource_path, file)
                 for file in os.listdir(env.user_resource_path)
                 if file.endswith(".ttf")]
    for ttf_file in resources:
        shutil.copy2(ttf_file, env.intermediate_path)

    shutil.copy2(
        os.path.join(env.user_resource_path, "WorldCoasts_obsolete.mwm"),
        env.mwm_path)

    for file in os.listdir(env.mwm_path):
        if file.startswith(WORLD_NAME) and file.endswith(".mwm"):
            resources.append(os.path.join(env.mwm_path, file))

    resources.sort()
    with open(env.external_resources_path, "w") as f:
        for resource in resources:
            fd = os.open(resource, os.O_RDONLY)
            f.write(f"{os.path.basename(resource)} {os.fstat(fd).st_size}")


@stage
def stage_localads(env):
    create_csv(env.localads_path, env.mwm_path, env.mwm_path, env.types_path,
               env.mwm_version, multiprocessing.cpu_count())
    make_tarfile(f"{env.localads_path}.tar.gz", env.localads_path)


@stage
def stage_statistics(env):
    result = defaultdict(lambda: defaultdict(dict))

    @country_stage_log
    def stage_mwm_statistics(env, country, **kwargs):
        stats_tmp = os.path.join(env.draft_path, f"{country}.stat")
        with open(stats_tmp, "w") as f:
            maps_stages.run_gen_tool_with_recovery_country(
                env,
                env.gen_tool,
                out=f,
                err=env.get_subprocess_out(country),
                data_path=env.mwm_path,
                user_resource_path=env.user_resource_path,
                type_statistics=True,
                output=country,
                **kwargs
            )
        result["countries"][country]["types"] = \
            make_stats(settings.STATS_TYPES_CONFIG, stats_tmp)

    mwms = env.get_mwm_names()
    countries = filter(lambda x: x not in WORLDS_NAMES, mwms)
    with ThreadPool() as pool:
        pool.map(partial(stage_mwm_statistics, env), countries)
    stages_info = get_stages_info(env.log_path, {"statistics"})
    result["stages"] = stages_info["stages"]
    for c in stages_info["countries"]:
        result["countries"][c]["stages"] = stages_info["countries"][c]

    def default(o):
        if isinstance(o, datetime.timedelta):
            return str(o)

    with open(os.path.join(env.stats_path, "stats.json"), "w") as f:
        json.dump(result, f, ensure_ascii=False, sort_keys=True,
                  indent=2, default=default)


@stage
def stage_cleanup(env):
    logger.info(f"osm2ft files will be moved from {env.out_path} "
                f"to {env.osm2ft_path}.")
    for x in os.listdir(env.mwm_path):
        p = os.path.join(env.mwm_path, x)
        if os.path.isfile(p) and x.endswith(".mwm.osm2ft"):
            shutil.move(p, os.path.join(env.osm2ft_path, x))

    logger.info(f"{env.draft_path} will be removed.")
    shutil.rmtree(env.draft_path)


MWM_STAGE = stage_mwm.__name__
COUNTRIES_STAGES = [s.__name__ for s in
                    (stage_index, stage_ugc, stage_popularity, stage_routing,
                     stage_routing_transit)]
STAGES = [s.__name__ for s in
          (stage_download_external, stage_download_production_external,
           stage_download_and_convert_planet, stage_update_planet,
           stage_coastline, stage_preprocess, stage_features, stage_mwm,
           stage_descriptions, stage_countries_txt, stage_external_resources,
           stage_localads, stage_statistics, stage_cleanup)]

ALL_STAGES = STAGES + COUNTRIES_STAGES


def stages_as_string(*args):
    return [x.__name__ for x in args]


def reset_to_stage(stage_name, env):
    def set_countries_stage(n):
        statuses = [os.path.join(env.status_path, f)
                    for f in os.listdir(env.status_path)
                    if os.path.isfile(os.path.join(env.status_path, f)) and
                    os.path.join(env.status_path, f) != env.main_status_path]
        for s in statuses:
            with open(s, "w") as f:
                f.write(n)

    _stage = f"stage_{stage_name}"
    stage_mwm_index = STAGES.index(MWM_STAGE)
    if _stage not in ALL_STAGES:
        raise ContinueError(
            f"Stage {stage_name} not in {', '.join(ALL_STAGES)}.")
    if not os.path.exists(env.main_status_path):
        raise ContinueError(f"Status file {env.main_status_path} not found.")
    if not os.path.exists(env.status_path):
        raise ContinueError(f"Status path {env.status_path} not found.")

    main_status = None
    if _stage in STAGES[:stage_mwm_index + 1]:
        main_status = _stage
        set_countries_stage(COUNTRIES_STAGES[0])
    elif _stage in STAGES[stage_mwm_index + 1:]:
        main_status = _stage
    elif _stage in COUNTRIES_STAGES:
        main_status = MWM_STAGE
        set_countries_stage(_stage)

    logger.info(f"New active status is {main_status}.")
    with open(env.main_status_path, "w") as f:
        f.write(main_status)


def generate_maps(env):
    stage_download_external(env)
    stage_download_production_external(env)
    with FileLock(planet_lock_file(), timeout=1) as planet_lock:
        stage_download_and_convert_planet(env)
        stage_update_planet(env)
        with FileLock(build_lock_file(env.out_path), timeout=1):
            stage_coastline(env)
            stage_preprocess(env)
            stage_features(env)
            planet_lock.release()
            stage_mwm(env)
            stage_descriptions(env)
            stage_countries_txt(env)
            stage_external_resources(env)
            stage_localads(env)
            stage_statistics(env)
            stage_cleanup(env)


def generate_coasts(env):
    with FileLock(planet_lock_file(), timeout=1) as planet_lock:
        stage_download_and_convert_planet(env)
        stage_update_planet(env)
        with FileLock(build_lock_file(env.out_path), timeout=1):
            stage_coastline(env)
            planet_lock.release()
            stage_cleanup(env)
