import logging
import os
import shutil
import subprocess

from . import settings
from .env import WORLD_NAME, WORLDS_NAMES
from .gen_tool import run_gen_tool
from .osmtools import osmconvert, osmupdate
from ..utils.file import download_file, is_verified
from ..utils.file import symlink_force
from ..utils.md5 import write_md5sum, md5

logger = logging.getLogger("maps_generator")


def download_planet(planet):
    download_file(settings.PLANET_URL, planet)
    download_file(settings.PLANET_MD5_URL, md5(planet))


def convert_planet(tool, in_planet, out_planet, output=subprocess.DEVNULL,
                   error=subprocess.DEVNULL):
    osmconvert(tool, in_planet, out_planet, output=output, error=error)
    write_md5sum(out_planet, md5(out_planet))


def stage_download_and_convert_planet(env, force_download, **kwargs):
    if force_download or not is_verified(settings.PLANET_PBF):
        download_planet(settings.PLANET_PBF)

    convert_planet(env[settings.OSM_TOOL_CONVERT],
                   settings.PLANET_PBF, settings.PLANET_O5M,
                   output=env.get_subprocess_out(),
                   error=env.get_subprocess_out())
    os.remove(settings.PLANET_PBF)
    os.remove(md5(settings.PLANET_PBF))


def stage_update_planet(env, **kwargs):
    tmp = settings.PLANET_O5M + ".tmp"
    osmupdate(env[settings.OSM_TOOL_UPDATE], settings.PLANET_O5M, tmp,
              output=env.get_subprocess_out(),
              error=env.get_subprocess_out(),
              **kwargs)
    os.remove(settings.PLANET_O5M)
    os.rename(tmp, settings.PLANET_O5M)
    write_md5sum(settings.PLANET_O5M, md5(settings.PLANET_O5M))


def stage_preprocess(env, **kwargs):
    run_gen_tool(env.gen_tool,
                 out=env.get_subprocess_out(),
                 err=env.get_subprocess_out(),
                 intermediate_data_path=env.intermediate_path,
                 osm_file_type="o5m",
                 osm_file_name=settings.PLANET_O5M,
                 node_storage=env.node_storage,
                 user_resource_path=env.user_resource_path,
                 preprocess=True,
                 **kwargs)


def stage_features(env, **kwargs):
    extra = {}
    if not env.production:
        extra["no_ads"] = True
    if any(x not in WORLDS_NAMES for x in env.countries):
        extra["generate_packed_borders"] = True
    if any(x == WORLD_NAME for x in env.countries):
        extra["generate_world"] = True
    if env.build_all_countries:
        extra["have_borders_for_whole_world"] = True

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
        **extra,
        **kwargs
    )

def run_gen_tool_with_recovery_country(env, *args, **kwargs):
    if "data_path" not in kwargs or "output" not in kwargs:
        logger.warning("The call run_gen_tool() will be without recovery.")
        run_gen_tool(*args, **kwargs)
    prev_data_path = kwargs["data_path"]
    mwm = f"{kwargs['output']}.mwm"
    osm2ft = f"{mwm}.osm2ft"
    kwargs["data_path"] = env.draft_path
    symlink_force(os.path.join(prev_data_path, osm2ft),
                  os.path.join(env.draft_path, osm2ft))
    shutil.copy(os.path.join(prev_data_path, mwm),
                os.path.join(env.draft_path, mwm))
    run_gen_tool(*args, **kwargs)
    shutil.move(os.path.join(env.draft_path, mwm),
                os.path.join(prev_data_path, mwm))
    kwargs["data_path"] = prev_data_path


def _generate_common_index(env, country, **kwargs):
    run_gen_tool(env.gen_tool,
                 out=env.get_subprocess_out(country),
                 err=env.get_subprocess_out(country),
                 data_path=env.mwm_path,
                 intermediate_data_path=env.intermediate_path,
                 user_resource_path=env.user_resource_path,
                 node_storage=env.node_storage,
                 planet_version=env.planet_version,
                 generate_geometry=True,
                 generate_index=True,
                 output=country,
                 **kwargs)


def stage_index_world(env, country, **kwargs):
    _generate_common_index(env, country,
                           generate_search_index=True,
                           cities_boundaries_data=env.cities_boundaries_path,
                           generate_cities_boundaries=True,
                           **kwargs)


def stage_cities_ids_world(env, country, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.mwm_path,
        user_resource_path=env.user_resource_path,
        output=country,
        generate_cities_ids=True,
        **kwargs
    )


def stage_index(env, country, **kwargs):
    _generate_common_index(env, country,
                           generate_search_index=True,
                           cities_boundaries_data=env.cities_boundaries_path,
                           generate_maxspeed=True,
                           make_city_roads=True,
                           **kwargs)


def stage_coastline_index(env, country, **kwargs):
    _generate_common_index(env, country, **kwargs)


def stage_ugc(env, country, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.mwm_path,
        intermediate_data_path=env.intermediate_path,
        user_resource_path=env.user_resource_path,
        ugc_data=env.ugc_path,
        output=country,
        **kwargs
    )


def stage_popularity(env, country, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.mwm_path,
        intermediate_data_path=env.intermediate_path,
        user_resource_path=env.user_resource_path,
        popular_places_data=env.popularity_path,
        generate_popular_places=True,
        output=country,
        **kwargs
    )


def stage_srtm(env, country, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.mwm_path,
        intermediate_data_path=env.intermediate_path,
        user_resource_path=env.user_resource_path,
        srtm_path=env.srtm_path,
        output=country,
        **kwargs
    )


def stage_routing(env, country, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.mwm_path,
        intermediate_data_path=env.intermediate_path,
        user_resource_path=env.user_resource_path,
        make_cross_mwm=True,
        disable_cross_mwm_progress=True,
        generate_cameras=True,
        make_routing_index=True,
        generate_traffic_keys=True,
        output=country,
        **kwargs
    )


def stage_routing_transit(env, country, **kwargs):
    run_gen_tool_with_recovery_country(
        env,
        env.gen_tool,
        out=env.get_subprocess_out(country),
        err=env.get_subprocess_out(country),
        data_path=env.mwm_path,
        intermediate_data_path=env.intermediate_path,
        user_resource_path=env.user_resource_path,
        transit_path=env.transit_path,
        make_transit_cross_mwm=True,
        output=country,
        **kwargs
    )
