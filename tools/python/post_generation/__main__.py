import argparse
import json
import sys

from post_generation.hierarchy_to_countries import (
    hierarchy_to_countries as hierarchy_to_countries_,
)


class PostGeneration:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description="Post-generation instruments",
            usage="""post_generation <command> [<args>]
The post_generation commands are:
    hierarchy_to_countries Produces countries.json from hierarchy.txt.
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
    def hierarchy_to_countries():
        parser = argparse.ArgumentParser(
            description="Produces countries.json from hierarchy.txt."
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
            help="Output countries.json file",
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
        with open(args.output, "w") as f:
            json.dump(countries, f, ensure_ascii=False, indent=1)


PostGeneration()
