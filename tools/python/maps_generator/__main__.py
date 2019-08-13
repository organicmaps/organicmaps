import logging
import os
from argparse import ArgumentParser, RawDescriptionHelpFormatter

from .generator import settings
from .generator.env import (Env, find_last_build_dir, WORLDS_NAMES,
                            WORLD_NAME, WORLD_COASTS_NAME)
from .generator.exceptions import ContinueError, SkipError, ValidationError
from .maps_generator import (generate_maps, generate_coasts, reset_to_stage,
                             ALL_STAGES, stage_download_production_external,
                             stage_descriptions, stage_ugc, stage_popularity,
                             stage_localads, stage_statistics, stage_srtm,
                             stages_as_string, stage_as_string, stage_coastline,
                             stage_update_planet)
from .utils.collections import unique

logger = logging.getLogger("maps_generator")


examples = """Examples:
1) Non-standard planet with coastlines
    If you want to generate maps for Japan you must complete the following steps:
    1. Open https://download.geofabrik.de/asia/japan.html and copy url of osm.pbf
     and md5sum files.
    2. Edit the ini file:
    maps_generator$ vim var/etc/map_generator.ini

    ...
    [Main]
    ...
    DEBUG: 0
    ...
    [External]
    PLANET_URL: https://download.geofabrik.de/asia/japan-latest.osm.pbf
    PLANET_MD5_URL: https://download.geofabrik.de/asia/japan-latest.osm.pbf.md5
    ...
    
    3. Run
    python$ python3.6 -m maps_generator --countries="World, WorldCoasts, Japan_*"

    You must skip the step of updating the planet, because it is a non-standard planet.
2) Rebuild stages:
    For example, you changed routing code in omim project and want to regenerate maps.
    You must have previous generation. You may regenerate from stage routing only for two mwms:
    
    python$ python3.6 -m maps_generator -c --from_stage="routing" --countries="Japan_Kinki Region_Osaka_Osaka, Japan_Chugoku Region_Tottori"
    
    Note: To generate maps with the coastline, you need more time and you need the planet to contain a continuous coastline.

3) Non-standard planet without coastlines
    If you want to generate maps for Moscow you must complete the following steps:
    1. Open https://download.geofabrik.de/russia/central-fed-district.html and copy url of osm.pbf and md5sum files.
    2. Edit ini file:
    maps_generator$ vim var/etc/map_generator.ini
    ...
    [Main]
    ...
    DEBUG: 0
    ...
    [External]
    PLANET_URL: https://download.geofabrik.de/russia/central-fed-district-latest.osm.pbf
    PLANET_MD5_URL: https://download.geofabrik.de/russia/central-fed-district-latest.osm.pbf.md5
    ...
    
    3. Run
    python$ python3.6 -m maps_generator --countries="Russia_Moscow" --skip="coastline"
"""


def parse_options():
    parser = ArgumentParser(description="Tool for generation maps for maps.me "
                                        "application.",
                            epilog=examples,
                            formatter_class=RawDescriptionHelpFormatter,
                            parents=[settings.parser])
    parser.add_argument(
        "-c",
        "--continue",
        default="",
        nargs="?",
        type=str,
        help="Continue the last build or specified in CONTINUE from the "
             "last stopped stage.")
    parser.add_argument(
        "--countries",
        type=str,
        default="",
        help="List of regions, separated by a comma or a semicolon, or path to "
             "file with regions, separated by a line break, for which maps"
             " will be built. The names of the regions can be seen "
             "in omim/data/borders. It is necessary to set names without "
             "any extension.")
    parser.add_argument(
        "--skip",
        type=str,
        default="",
        help=f"List of stages, separated by a comma or a semicolon, "
        f"for which building will be skipped. Available skip stages: "
        f"{', '.join([s.replace('stage_', '') for s in ALL_STAGES])}.")
    parser.add_argument(
        "--from_stage",
        type=str,
        default="",
        help=f"Stage from which maps will be rebuild. Available stages: "
        f"{', '.join([s.replace('stage_', '') for s in ALL_STAGES])}.")
    parser.add_argument(
        "--coasts",
        default=False,
        action="store_true",
        help="Build only WorldCoasts.raw and WorldCoasts.rawgeom files")
    parser.add_argument(
        "--production",
        default=False,
        action="store_true",
        help="Build production maps. In another case, 'osm only maps' are built"
             " - maps without additional data and advertising.")
    parser.add_argument(
        "--order",
        type=str,
        default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             "var/etc/file_generation_order.txt"),
        help="Mwm generation order.")
    return vars(parser.parse_args())


def main():
    root = logging.getLogger()
    root.addHandler(logging.NullHandler())
    options = parse_options()

    build_name = None
    if options["continue"] is None or options["continue"]:
        d = find_last_build_dir(options["continue"])
        if d is None:
            raise ContinueError("The build cannot continue: the last build "
                                "directory was not found.")
        build_name = d
    options["build_name"] = build_name

    countries_line = ""
    if "COUNTRIES" in os.environ:
        countries_line = os.environ["COUNTRIES"]
    if options["countries"]:
        options["build_all_countries"] = False
        countries_line = options["countries"]
    else:
        options["build_all_countries"] = True
    raw_countries = []
    if os.path.isfile(countries_line):
        with open(countries_line) as f:
            raw_countries = [x.strip() for x in f]
    if countries_line:
        raw_countries = [
            x.strip() for x in countries_line.replace(";", ",").split(",")
        ]

    borders_path = os.path.join(settings.USER_RESOURCE_PATH, "borders")
    all_countries = [
        f.replace(".poly", "") for f in os.listdir(borders_path)
        if os.path.isfile(os.path.join(borders_path, f))
    ]
    all_countries += list(WORLDS_NAMES)
    countries = []
    used_countries = set()

    def end_star_compare(prefix, full):
        return full.startswith(prefix)

    def compare(a, b):
        return a == b

    for raw_country in raw_countries:
        cmp = compare
        _raw_country = raw_country[:]
        if _raw_country and _raw_country[-1] == "*":
            _raw_country = _raw_country.replace("*", "")
            cmp = end_star_compare

        for country in all_countries:
            if cmp(_raw_country, country):
                used_countries.add(raw_country)
                countries.append(country)

    countries = unique(countries)
    diff = set(raw_countries) - used_countries
    if diff:
        raise ValidationError(f"Bad input countries {', '.join(diff)}")
    options["countries"] = countries if countries else all_countries

    if options["order"]:
        ordered_countries = []
        countries = set(options["countries"])
        with open(options["order"]) as file:
            for c in file:
                if c.strip().startswith("#"):
                    continue
                c = c.split("\t")[0].strip()
                if c in countries:
                    ordered_countries.append(c)
                    countries.remove(c)
            if countries:
                raise ValueError(f"{options['order']} does not have an order "
                                 f"for {countries}.")
        options["countries"] = ordered_countries

    options_skip = []
    if options["skip"]:
        options_skip = [
            f"stage_{s.strip()}"
            for s in options["skip"].replace(";", ",").split(",")
        ]
    options["skip"] = options_skip
    if not options["production"]:
        options["skip"] += stages_as_string(
            stage_download_production_external,
            stage_ugc,
            stage_popularity,
            stage_srtm,
            stage_descriptions,
            stage_localads,
            stage_statistics
        )
    if not all(s in ALL_STAGES for s in options["skip"]):
        raise SkipError(f"Stages {set(options['skip']) - set(ALL_STAGES)} "
                        f"not found.")

    if settings.PLANET_URL != settings.DEFAULT_PLANET_URL:
        options["skip"] += stages_as_string(stage_update_planet)

    if stage_as_string(stage_coastline) in options["skip"]:
        worlds_names = [x for x in options["countries"] if x in WORLDS_NAMES]
        if worlds_names:
            raise SkipError(f"You can not skip {stages_as_string(stage_coastline)}"
                            f" if you want to generate {countries}")

    env = Env(options)
    if env.from_stage:
        reset_to_stage(env.from_stage, env)
    if env.coasts:
        generate_coasts(env)
    else:
        generate_maps(env)
    env.finish()


main()
