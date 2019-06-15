import logging
import os
import shutil

from .gen_tool import run_gen_tool
from ..utils.file import symlink_force

logger = logging.getLogger("maps_generator")


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


def stage_index_world(env, country, **kwargs):
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
                 generate_search_index=True,
                 cities_boundaries_data=env.cities_boundaries_path,
                 make_city_roads=True,
                 output=country,
                 **kwargs)


def stage_index(env, country, **kwargs):
    stage_index_world(env, country, generate_maxspeed=True, **kwargs)


def stage_coastline_index(env, country, **kwargs):
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
