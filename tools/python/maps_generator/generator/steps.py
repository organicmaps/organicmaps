"""
This file contains basic api for generator_tool and osm tools to generate maps.
"""
import functools
import logging
import os
import shutil
import subprocess
from typing import AnyStr

from maps_generator.generator import settings
from maps_generator.generator.env import Env
from maps_generator.generator.env import PathProvider
from maps_generator.generator.env import WORLDS_NAMES
from maps_generator.generator.env import WORLD_NAME
from maps_generator.generator.env import get_all_countries_list
from maps_generator.generator.gen_tool import run_gen_tool
from maps_generator.generator.osmtools import osmconvert
from maps_generator.generator.osmtools import osmupdate
from maps_generator.utils.file import download_file
from maps_generator.utils.file import is_verified
from maps_generator.utils.file import symlink_force
from maps_generator.utils.md5 import md5
from maps_generator.utils.md5 import write_md5sum

logger = logging.getLogger("maps_generator")


def multithread_run_if_one_country(func):
    @functools.wraps(func)
    def wrap(env, *args, **kwargs):
        if len(env.countries) == 1:
            kwargs.update({"threads_count": settings.THREADS_COUNT})
        func(env, *args, **kwargs)

    return wrap


def download_planet(planet: AnyStr):
    download_file(settings.PLANET_URL, planet)
    download_file(settings.PLANET_MD5_URL, md5(planet))


def convert_planet(
    tool: AnyStr,
    in_planet: AnyStr,
    out_planet: AnyStr,
    output=subprocess.DEVNULL,
    error=subprocess.DEVNULL,
):
    osmconvert(tool, in_planet, out_planet, output=output, error=error)
    write_md5sum(out_planet, md5(out_planet))


def step_download_and_convert_planet(env: Env, force_download: bool, **kwargs):
    if force_download or not is_verified(env.paths.planet_osm_pbf):
        download_planet(env.paths.planet_osm_pbf)

    convert_planet(
        env[settings.OSM_TOOL_CONVERT],
        env.paths.planet_osm_pbf,
        env.paths.planet_o5m,
        output=env.get_subprocess_out(),
        error=env.get_subprocess_out(),
    )
    os.remove(env.paths.planet_osm_pbf)
    os.remove(md5(env.paths.planet_osm_pbf))


def step_update_planet(env: Env, **kwargs):
    tmp = f"{env.paths.planet_o5m}.tmp"
    osmupdate(
        env[settings.OSM_TOOL_UPDATE],
        env.paths.planet_o5m,
        tmp,
        output=env.get_subprocess_out(),
        error=env.get_subprocess_out(),
        **kwargs,
    )
    os.remove(env.paths.planet_o5m)
    os.rename(tmp, env.paths.planet_o5m)
    write_md5sum(env.paths.planet_o5m, md5(env.paths.planet_o5m))


def step_preprocess(env: Env, **kwargs):
    run_gen_tool(
        env.gen_tool,
        out=env.get_subprocess_out(),
        err=env.get_subprocess_out(),
        intermediate_data_path=env.paths.intermediate_data_path,
        osm_file_type="o5m",
        osm_file_name=env.paths.planet_o5m,
        node_storage=env.node_storage,
        user_resource_path=env.paths.user_resource_path,
        preprocess=True,
        **kwargs,
    )


def step_features(env: Env, **kwargs):
    if env.production:
        kwargs.update({"add_ads": True})
    if any(x not in WORLDS_NAMES for x in env.countries):
        kwargs.update({"generate_packed_borders": True})
    if any(x == WORLD_NAME for x in env.countries):
        kwargs.update({"generate_world": True})
    if len(env.countries) == len(get_all_countries_list(PathProvider.borders_path())):
        kwargs.update({"have_borders_for_whole_world": True})

    run_gen_tool(
        env.gen_tool,
        out=env.get_subprocess_out(),
        err=env.get_subprocess_out(),
        data_path=env.paths.data_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        osm_file_type="o5m",
        osm_file_name=env.paths.planet_o5m,
        node_storage=env.node_storage,
        user_resource_path=env.paths.user_resource_path,
        dump_cities_boundaries=True,
        cities_boundaries_data=env.paths.cities_boundaries_path,
        generate_features=True,
        threads_count=settings.THREADS_COUNT,
        **kwargs,
    )


def run_gen_tool_with_recovery_country(env: Env, *args, **kwargs):
    if "data_path" not in kwargs or "output" not in kwargs:
        logger.warning("The call run_gen_tool() will be without recovery.")
        run_gen_tool(*args, **kwargs)

    prev_data_path = kwargs["data_path"]
    mwm = f"{kwargs['output']}.mwm"
    osm2ft = f"{mwm}.osm2ft"
    kwargs["data_path"] = env.paths.draft_path
    symlink_force(
        os.path.join(prev_data_path, osm2ft), os.path.join(env.paths.draft_path, osm2ft)
    )
    shutil.copy(
        os.path.join(prev_data_path, mwm), os.path.join(env.paths.draft_path, mwm)
    )
    run_gen_tool(*args, **kwargs)
    shutil.move(
        os.path.join(env.paths.draft_path, mwm), os.path.join(prev_data_path, mwm)
    )
    kwargs["data_path"] = prev_data_path


@multithread_run_if_one_country
def _generate_common_index(env: Env, country: AnyStr, **kwargs):
    run_gen_tool(
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        node_storage=env.node_storage,
        planet_version=env.planet_version,
        generate_geometry=True,
        generate_index=True,
        output=country,
        **kwargs,
    )


def step_index_world(env: Env, country: AnyStr, **kwargs):
    _generate_common_index(
        env,
        country,
        generate_search_index=True,
        cities_boundaries_data=env.paths.cities_boundaries_path,
        generate_cities_boundaries=True,
        **kwargs,
    )


def step_cities_ids_world(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        user_resource_path=env.paths.user_resource_path,
        output=country,
        generate_cities_ids=True,
        **kwargs,
    )


def step_index(env: Env, country: AnyStr, **kwargs):
    _generate_common_index(env, country, generate_search_index=True, **kwargs)


def step_coastline_index(env: Env, country: AnyStr, **kwargs):
    _generate_common_index(env, country, **kwargs)


def step_ugc(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        ugc_data=env.paths.ugc_path,
        output=country,
        **kwargs,
    )


def step_popularity(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        popular_places_data=env.paths.popularity_path,
        generate_popular_places=True,
        output=country,
        **kwargs,
    )


def step_srtm(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        srtm_path=env.paths.srtm_path(),
        output=country,
        **kwargs,
    )


def step_isolines_info(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        generate_isolines_info=True,
        isolines_path=PathProvider.isolines_path(),
        output=country,
        **kwargs,
    )


def step_description(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        user_resource_path=env.paths.user_resource_path,
        wikipedia_pages=env.paths.descriptions_path,
        idToWikidata=env.paths.id_to_wikidata_path,
        output=country,
        **kwargs,
    )


def step_routing(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        cities_boundaries_data=env.paths.cities_boundaries_path,
        generate_maxspeed=True,
        make_city_roads=True,
        make_cross_mwm=True,
        disable_cross_mwm_progress=True,
        generate_cameras=True,
        make_routing_index=True,
        generate_traffic_keys=True,
        output=country,
        **kwargs,
    )


def step_routing_transit(env: Env, country: AnyStr, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.paths.mwm_path,
        intermediate_data_path=env.paths.intermediate_data_path,
        user_resource_path=env.paths.user_resource_path,
        transit_path=env.paths.transit_path,
        make_transit_cross_mwm=True,
        output=country,
        **kwargs,
    )
