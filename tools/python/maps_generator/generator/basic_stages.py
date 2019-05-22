import os
import subprocess

from ..utils.file import download_file, is_verified
from ..utils.md5 import write_md5sum, md5
from . import settings
from .gen_tool import run_gen_tool
from .osmtools import osmconvert, osmupdate
from .exceptions import wait_and_raise_if_fail


def download_planet(planet, output=subprocess.DEVNULL,
                    error=subprocess.DEVNULL):
    p = download_file(settings.PLANET_URL, planet, output=output, error=error)
    m = download_file(settings.PLANET_MD5_URL, md5(planet), output=output,
                      error=error)
    wait_and_raise_if_fail(p)
    wait_and_raise_if_fail(m)


def convert_planet(tool, in_planet, out_planet, output=subprocess.DEVNULL,
                   error=subprocess.DEVNULL):
    osmconvert(tool, in_planet, out_planet, output=output, error=error)
    write_md5sum(out_planet, md5(out_planet))


def stage_download_and_convert_planet(env, **kwargs):
    if not is_verified(settings.PLANET_PBF):
        download_planet(settings.PLANET_PBF, output=env.get_subprocess_out(),
                        error=env.get_subprocess_out())

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
                 preprocess=True,
                 **kwargs)
