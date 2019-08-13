import os
import subprocess

from . import settings
from .gen_tool import run_gen_tool
from .osmtools import osmfilter


def filter_coastline(name_executable, in_file, out_file,
                     output=subprocess.DEVNULL, error=subprocess.DEVNULL):
    osmfilter(name_executable, in_file, out_file, output=output,
              error=error, keep="", keep_ways="natural=coastline",
              keep_nodes="capital=yes place=town =city")


def make_coastline(env):
    coastline_o5m = os.path.join(env.coastline_path, "coastline.o5m")
    filter_coastline(env[settings.OSM_TOOL_FILTER], settings.PLANET_O5M,
                     coastline_o5m, output=env.get_subprocess_out(),
                     error=env.get_subprocess_out())

    run_gen_tool(env.gen_tool,
                 out=env.get_subprocess_out(),
                 err=env.get_subprocess_out(),
                 intermediate_data_path=env.coastline_path,
                 osm_file_type="o5m",
                 osm_file_name=coastline_o5m,
                 node_storage=env.node_storage,
                 user_resource_path=env.user_resource_path,
                 preprocess=True)

    run_gen_tool(env.gen_tool,
                 out=env.get_subprocess_out(),
                 err=env.get_subprocess_out(),
                 intermediate_data_path=env.coastline_path,
                 osm_file_type="o5m",
                 osm_file_name=coastline_o5m,
                 node_storage=env.node_storage,
                 user_resource_path=env.user_resource_path,
                 make_coasts=True,
                 fail_on_coasts=True)
