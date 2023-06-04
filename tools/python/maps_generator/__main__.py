import logging
import os
from argparse import ArgumentParser
from argparse import RawDescriptionHelpFormatter

from maps_generator.generator import settings
from maps_generator.generator import stages
from maps_generator.generator import stages_declaration as sd
from maps_generator.generator.env import Env
from maps_generator.generator.env import PathProvider
from maps_generator.generator.env import WORLDS_NAMES
from maps_generator.generator.env import find_last_build_dir
from maps_generator.generator.env import get_all_countries_list
from maps_generator.generator.exceptions import ContinueError
from maps_generator.generator.exceptions import SkipError
from maps_generator.generator.exceptions import ValidationError
from maps_generator.maps_generator import generate_coasts
from maps_generator.maps_generator import generate_maps
from maps_generator.utils.algo import unique

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
    
    python$ python3.6 -m maps_generator -c --from_stage="Routing" --countries="Japan_Kinki Region_Osaka_Osaka, Japan_Chugoku Region_Tottori"
    
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
    python$ python3.6 -m maps_generator --countries="Russia_Moscow" --skip="Coastline"
"""


def parse_options():
    parser = ArgumentParser(
        description="Tool for generation maps for maps.me " "application.",
        epilog=examples,
        formatter_class=RawDescriptionHelpFormatter,
        parents=[settings.parser],
    )
    parser.add_argument(
        "-c",
        "--continue",
        default="",
        nargs="?",
        type=str,
        help="Continue the last build or specified in CONTINUE from the "
        "last stopped stage.",
    )
    parser.add_argument(
        "-s",
        "--suffix",
        default="",
        type=str,
        help="Suffix of the name of a build directory.",
    )
    parser.add_argument(
        "--countries",
        type=str,
        default="",
        help="List of regions, separated by a comma or a semicolon, or path to "
        "file with regions, separated by a line break, for which maps"
        " will be built. The names of the regions can be seen "
        "in omim/data/borders. It is necessary to set names without "
        "any extension.",
    )
    parser.add_argument(
        "--without_countries",
        type=str,
        default="",
        help="List of regions to exclude them from generation. Syntax is the same as for --countries.",
    )
    parser.add_argument(
        "--skip",
        type=str,
        default="",
        help=f"List of stages, separated by a comma or a semicolon, "
        f"for which building will be skipped. Available skip stages: "
        f"{', '.join([s.replace('stage_', '') for s in stages.stages.get_visible_stages_names()])}.",
    )
    parser.add_argument(
        "--from_stage",
        type=str,
        default="",
        help=f"Stage from which maps will be rebuild. Available stages: "
        f"{', '.join([s.replace('stage_', '') for s in stages.stages.get_visible_stages_names()])}.",
    )
    parser.add_argument(
        "--coasts",
        default=False,
        action="store_true",
        help="Build only WorldCoasts.raw and WorldCoasts.rawgeom files",
    )
    parser.add_argument(
        "--force_download_files",
        default=False,
        action="store_true",
        help="If build is continued, files will always be downloaded again.",
    )
    parser.add_argument(
        "--production",
        default=False,
        action="store_true",
        help="Build production maps. In another case, 'osm only maps' are built"
        " - maps without additional data and advertising.",
    )
    parser.add_argument(
        "--order",
        type=str,
        default=os.path.join(
            os.path.dirname(os.path.abspath(__file__)),
            "var/etc/file_generation_order.txt",
        ),
        help="Mwm generation order.",
    )
    return parser.parse_args()


def main():
    root = logging.getLogger()
    root.addHandler(logging.NullHandler())

    options = parse_options()

    # Processing of 'continue' option.
    # If 'continue' is set maps generation is continued from the last build
    # that is found automatically.
    build_name = None
    continue_ = getattr(options, "continue")
    if continue_ is None or continue_:
        d = find_last_build_dir(continue_)
        if d is None:
            raise ContinueError(
                "The build cannot continue: the last build " "directory was not found."
            )
        build_name = d

    # Processing of 'countries' option.
    # There is processing 'countries' and 'without_countries' options.
    # Option 'without_countries' has more priority than 'countries'.
    # Options 'countries' and 'without_countries' can include '*'.
    # For example: '--countries="UK*, Japan"*' means
    # '--countries="UK_England_East Midlands, UK_England_East of England_Essex, ...,
    # Japan_Chubu Region_Aichi_Nagoya, Japan_Chubu Region_Aichi_Toyohashi, ..."'.
    countries_line = ""
    without_countries_line = ""
    if "COUNTRIES" in os.environ:
        countries_line = os.environ["COUNTRIES"]
    if options.countries:
        countries_line = options.countries
    else:
        countries_line = "*"

    if options.without_countries:
        without_countries_line = options.without_countries

    all_countries = get_all_countries_list(PathProvider.borders_path())

    def end_star_compare(prefix, full):
        return full.startswith(prefix)

    def compare(a, b):
        return a == b

    def get_countries_set_from_line(line):
        countries = []
        used_countries = set()
        countries_list = []
        if os.path.isfile(line):
            with open(line) as f:
                countries_list = [x.strip() for x in f]
        elif line:
            countries_list = [x.strip() for x in line.replace(";", ",").split(",")]

        for country_item in countries_list:
            cmp = compare
            _raw_country = country_item[:]
            if _raw_country and _raw_country[-1] == "*":
                _raw_country = _raw_country.replace("*", "")
                cmp = end_star_compare

            for country in all_countries:
                if cmp(_raw_country, country):
                    used_countries.add(country_item)
                    countries.append(country)

        countries = unique(countries)
        diff = set(countries_list) - used_countries
        if diff:
            raise ValidationError(f"Bad input countries {', '.join(diff)}")
        return set(countries)

    countries = get_countries_set_from_line(countries_line)
    without_countries = get_countries_set_from_line(without_countries_line)
    countries -= without_countries
    countries = list(countries)
    if not countries:
        countries = all_countries

    # Processing of 'order' option.
    # It defines an order of countries generation using a file from 'order' path.
    if options.order:
        ordered_countries = []
        countries = set(countries)
        with open(options.order) as file:
            for c in file:
                if c.strip().startswith("#"):
                    continue
                c = c.split("\t")[0].strip()
                if c in countries:
                    ordered_countries.append(c)
                    countries.remove(c)
            if countries:
                raise ValueError(
                    f"{options.order} does not have an order " f"for {countries}."
                )
        countries = ordered_countries

    # Processing of 'skip' option.
    skipped_stages = set()
    if options.skip:
        for s in options.skip.replace(";", ",").split(","):
            stage = s.strip()
            if not stages.stages.is_valid_stage_name(stage):
                raise SkipError(f"{stage} not found.")
            skipped_stages.add(stages.get_stage_type(stage))

    if settings.PLANET_URL != settings.DEFAULT_PLANET_URL:
        skipped_stages.add(sd.StageUpdatePlanet)

    if sd.StageCoastline in skipped_stages:
        if any(x in WORLDS_NAMES for x in options.countries):
            raise SkipError(
                f"You can not skip {stages.get_stage_name(sd.StageCoastline)}"
                f" if you want to generate {WORLDS_NAMES}."
                f" You can exclude them with --without_countries option."
            )

    if not settings.NEED_PLANET_UPDATE:
        skipped_stages.add(sd.StageUpdatePlanet)

    if not settings.NEED_BUILD_WORLD_ROADS:
        skipped_stages.add(sd.StageRoutingWorld)

    # Make env and run maps generation.
    env = Env(
        countries=countries,
        production=options.production,
        build_name=build_name,
        build_suffix=options.suffix,
        skipped_stages=skipped_stages,
        force_download_files=options.force_download_files
    )
    from_stage = None
    if options.from_stage:
        from_stage = f"{options.from_stage}"
    if options.coasts:
        generate_coasts(env, from_stage)
    else:
        generate_maps(env, from_stage)
    env.finish()


main()
