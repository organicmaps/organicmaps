import argparse
import os
import sys

from .hierarchy_to_countries import hierarchy_to_countries as hierarchy_to_countries_
from .localads_mwm_to_csv import create_csv


class PostGeneration:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description="Post-generation instruments",
            usage="""post_generation <command> [<args>]
The post_generation commands are:
    localads_mwm_to_csv    Prepares CSV files for uploading to localads database from mwm files.
    hierarchy_to_countries Produces countries.txt from hierarchy.txt.
    """)
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
                        "from mwm files.")
        parser.add_argument("mwm", help="path to mwm files")
        parser.add_argument(
            "--osm2ft",
            help="path to osm2ft files (default is the same as mwm)")
        parser.add_argument("--output",
                            default=".",
                            help="path to generated files ('.' by default)")
        types_default = os.path.join(os.path.dirname(__file__), "..", "..",
                                     "..", "data", "types.txt")
        parser.add_argument("--types",
                            default=types_default,
                            help="path to omim/data/types.txt")
        parser.add_argument("--threads",
                            type=int,
                            default=1,
                            help="number of threads to process files")
        parser.add_argument("--mwm_version", type=int, required=True,
                            help="Mwm version")
        args = parser.parse_args(sys.argv[2:])
        if not args.osm2ft:
            args.osm2ft = args.mwm

        create_csv(args.output, args.mwm, args.osm2ft, args.types,
                   args.mwm_version, args.threads)

    @staticmethod
    def hierarchy_to_countries():
        parser = argparse.ArgumentParser(
            description="Produces countries.txt from hierarchy.txt.")
        parser.add_argument("--target", required=True,
                            help="Path to mwm files")
        parser.add_argument("--hierarchy", required=True,
                            default="hierarchy.txt",
                            help="Hierarchy file")
        parser.add_argument("--old", required=True,
                            help="old_vs_new.csv file")
        parser.add_argument("--osm", required=True,
                            help="borders_vs_osm.csv file")
        parser.add_argument("--mwm_version", type=int, required=True,
                            help="Mwm version")
        parser.add_argument("-o", "--output", required=True,
                            help="Output countries.txt file (default is stdout)")
        args = parser.parse_args(sys.argv[2:])
        countries_json = hierarchy_to_countries_(args.old, args.osm,
                                                 args.hierarchy,
                                                 args.target,
                                                 args.mwm_version)
        if args.output:
            with open(args.output, "w") as f:
                f.write(countries_json)
        else:
            print(countries_json)


PostGeneration()
