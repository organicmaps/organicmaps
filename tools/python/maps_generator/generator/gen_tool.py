import copy
import logging
import os
import subprocess

from maps_generator.generator.exceptions import OptionNotFound
from maps_generator.generator.exceptions import ValidationError
from maps_generator.generator.exceptions import wait_and_raise_if_fail

logger = logging.getLogger("maps_generator")


class GenTool:
    OPTIONS = {
        "add_ads": bool,
        "disable_cross_mwm_progress": bool,
        "dump_cities_boundaries": bool,
        "emit_coasts": bool,
        "fail_on_coasts": bool,
        "generate_cameras": bool,
        "generate_cities_boundaries": bool,
        "generate_cities_ids": bool,
        "generate_features": bool,
        "generate_geo_objects_features": bool,
        "generate_geo_objects_index": bool,
        "generate_geometry": bool,
        "generate_index": bool,
        "generate_isolines_info": bool,
        "generate_maxspeed": bool,
        "generate_packed_borders": bool,
        "generate_popular_places": bool,
        "generate_region_features": bool,
        "generate_regions": bool,
        "generate_regions_kv": bool,
        "generate_search_index": bool,
        "generate_traffic_keys": bool,
        "generate_world": bool,
        "have_borders_for_whole_world": bool,
        "make_city_roads": bool,
        "make_coasts": bool,
        "make_cross_mwm": bool,
        "make_routing_index": bool,
        "make_transit_cross_mwm": bool,
        "make_transit_cross_mwm_experimental": bool,
        "preprocess": bool,
        "split_by_polygons": bool,
        "type_statistics": bool,
        "version": bool,
        "threads_count": int,
        "booking_data": str,
        "promo_catalog_cities": str,
        "brands_data": str,
        "brands_translations_data": str,
        "cache_path": str,
        "cities_boundaries_data": str,
        "data_path": str,
        "dump_wikipedia_urls": str,
        "geo_objects_features": str,
        "geo_objects_key_value": str,
        "ids_without_addresses": str,
        "idToWikidata": str,
        "intermediate_data_path": str,
        "isolines_path": str,
        "nodes_list_path": str,
        "node_storage": str,
        "osm_file_name": str,
        "osm_file_type": str,
        "output": str,
        "planet_version": str,
        "popular_places_data": str,
        "regions_features": str,
        "regions_index": str,
        "regions_key_value": str,
        "srtm_path": str,
        "transit_path": str,
        "transit_path_experimental": str,
        "world_roads_path": str,
        "ugc_data": str,
        "uk_postcodes_dataset": str,
        "us_postcodes_dataset": str,
        "user_resource_path": str,
        "wikipedia_pages": str,
    }

    def __init__(
        self, name_executable, out=subprocess.DEVNULL, err=subprocess.DEVNULL, **options
    ):
        self.name_executable = name_executable
        self.subprocess = None
        self.output = out
        self.error = err
        self.options = {"threads_count": 1}
        self.logger = logger
        self.add_options(**options)

    @property
    def args(self):
        return self._collect_cmd()

    def add_options(self, **options):
        if "logger" in options:
            self.logger = options["logger"]

        for k, v in options.items():
            if k == "logger":
                continue

            if k not in GenTool.OPTIONS:
                raise OptionNotFound(f"{k} is unavailable option")

            if type(v) is not GenTool.OPTIONS[k]:
                raise ValidationError(
                    f"{k} required {str(GenTool.OPTIONS[k])},"
                    f" but not {str(type(v))}"
                )

            self.options[k] = str(v).lower() if type(v) is bool else v
        return self

    def run_async(self):
        assert self.subprocess is None, "You forgot to call wait()"
        cmd = self._collect_cmd()
        self.subprocess = subprocess.Popen(
            cmd, stdout=self.output, stderr=self.error, env=os.environ
        )

        self.logger.info(
            f"Run generator tool [{self.get_build_version()}]:" f" {' '.join(cmd)} "
        )
        return self

    def wait(self):
        code = self.subprocess.wait()
        self.subprocess = None
        return code

    def run(self):
        self.run_async()
        wait_and_raise_if_fail(self)

    def branch(self):
        c = GenTool(self.name_executable, out=self.output, err=self.error)
        c.options = copy.deepcopy(self.options)
        return c

    def get_build_version(self):
        p = subprocess.Popen(
            [self.name_executable, "--version"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            env=os.environ,
        )
        wait_and_raise_if_fail(p)
        out, err = p.communicate()
        return out.decode("utf-8").replace("\n", " ").strip()

    def _collect_cmd(self):
        options = ["".join(["--", k, "=", str(v)]) for k, v in self.options.items()]
        return [self.name_executable, *options]


def run_gen_tool(*args, **kwargs):
    GenTool(*args, **kwargs).run()
