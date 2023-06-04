import argparse
import json
import os
import sys

from post_generation.hierarchy_to_countries import (
    hierarchy_to_countries as hierarchy_to_countries_,
)
from post_generation.inject_promo_ids import inject_promo_ids
from post_generation.localads_mwm_to_csv import create_csv


class PostGeneration:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description="Post-generation instruments",
            usage="""post_generation <command> [<args>]
The post_generation commands are:
    localads_mwm_to_csv    Prepares CSV files for uploading to localads database from mwm files.
    hierarchy_to_countries Produces countries.txt from hierarchy.txt.
    inject_promo_ids       Injects promo osm ids into countries.txt
    """,
        )
        parser.add_argument("command", help="Subcommand to run")
        args = parser.parse_args(sys.argv[1:2])
        if not hasattr(self, args.command):
            print(f"Unrecognized command {args.command}")
            parser.print_help()
            exit(1)
        getattr(self, args.command)()

    @staticmethod
    def localads_mwm_to_csv():
        parser = argparse.ArgumentParser(
            description="Prepares CSV files for uploading to localads database "
            "from mwm files."
        )
        parser.add_argument("mwm", help="path to mwm files")
        parser.add_argument(
            "--osm2ft", help="path to osm2ft files (default is the same as mwm)"
        )
        parser.add_argument(
            "--output", default=".", help="path to generated files ('.' by default)"
        )
        types_default = os.path.join(
            os.path.dirname(__file__), "..", "..", "..", "data", "types.txt"
        )
        parser.add_argument(
            "--types", default=types_default, help="path to omim/data/types.txt"
        )
        parser.add_argument(
            "--threads", type=int, default=1, help="number of threads to process files"
        )
        parser.add_argument(
            "--mwm_version", type=int, required=True, help="Mwm version"
        )
        args = parser.parse_args(sys.argv[2:])
        if not args.osm2ft:
            args.osm2ft = args.mwm

        create_csv(
            args.output,
            args.mwm,
            args.osm2ft,
            args.mwm_version,
            args.threads,
        )

    @staticmethod
    def hierarchy_to_countries():
        parser = argparse.ArgumentParser(
            description="Produces countries.txt from hierarchy.txt."
        )
        parser.add_argument("--target", required=True, help="Path to mwm files")
        parser.add_argument(
            "--hierarchy", required=True, default="hierarchy.txt", help="Hierarchy file"
        )
        parser.add_argument("--old", required=True, help="old_vs_new.csv file")
        parser.add_argument("--osm", required=True, help="borders_vs_osm.csv file")
        parser.add_argument(
            "--countries_synonyms", required=True, help="countries_synonyms.csv file"
        )
        parser.add_argument(
            "--mwm_version", type=int, required=True, help="Mwm version"
        )
        parser.add_argument(
            "-o",
            "--output",
            required=True,
            help="Output countries.txt file (default is stdout)",
        )
        args = parser.parse_args(sys.argv[2:])
        countries = hierarchy_to_countries_(
            args.old,
            args.osm,
            args.countries_synonyms,
            args.hierarchy,
            args.target,
            args.mwm_version,
        )
        if args.output:
            with open(args.output, "w") as f:
                json.dump(countries, f, ensure_ascii=True, indent=1)
        else:
            print(json.dumps(countries, ensure_ascii=True, indent=1))

    @staticmethod
    def inject_promo_ids():
        parser = argparse.ArgumentParser(
            description="Injects promo cities osm ids into countries.txt"
        )
        parser.add_argument("--mwm", required=True, help="path to mwm files")
        parser.add_argument(
            "--types", required=True, help="path to omim/data/types.txt"
        )
        parser.add_argument(
            "--promo_cities", required=True, help="Path to promo cities file"
        )
        parser.add_argument(
            "--promo_countries", required=True, help="Path to promo countries file"
        )
        parser.add_argument(
            "--osm2ft", help="path to osm2ft files (default is the same as mwm)"
        )
        parser.add_argument(
            "--countries",
            help="path to countries.txt file (default is countries.txt file into mwm directory)",
        )
        parser.add_argument(
            "--output",
            help="Output countries.txt file (default is countries.txt file into mwm directory)",
        )
        args = parser.parse_args(sys.argv[2:])

        if not args.osm2ft:
            args.osm2ft = args.mwm
        if not args.countries:
            args.countries = os.path.join(args.mwm, "countries.txt")
        if not args.output:
            args.output = os.path.join(args.mwm, "countries.txt")

        with open(args.countries) as f:
            countries = json.load(f)

        inject_promo_ids(
            countries,
            args.promo_cities,
            args.promo_countries,
            args.mwm,
            args.types,
            args.osm2ft,
        )

        with open(args.output, "w") as f:
            json.dump(countries, f, ensure_ascii=True, indent=1)


PostGeneration()
