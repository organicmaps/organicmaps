import argparse
import logging
import sys

from mwm.decode_id import decode_id
from mwm.dump_mwm import dump_mwm
from mwm.find_feature import find_and_print_features
from mwm.ft2osm import ft2osm
from mwm.mwm_feature_compare import compare_mwm

logger = logging.getLogger("mwm")
logger.setLevel(logging.ERROR)


class Mwm:
    def __init__(self):
        parser = argparse.ArgumentParser(
            description="Mwm utils",
            usage="""mwm <command> [<args>]
The most commonly used mwm commands are:
    decode_id            Unpacks maps.me OSM id to an OSM object link.
    dump_mwm             Dumps some MWM structures.
    find_feature         Finds features in an mwm file based on a query.
    ft2osm               Finds an OSM object for a given feature id.
    mwm_feature_compare  Compares feature count in .mwm files.
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
    def decode_id():
        parser = argparse.ArgumentParser(
            description="Unpacks maps.me OSM id to an OSM object link."
        )
        parser.add_argument(
            "--id", type=str, required=True, help="OsmId or url from osm.org."
        )
        args = parser.parse_args(sys.argv[2:])
        id = decode_id(args.id)
        if id is None:
            print("Decode id error.")
            exit(1)
        print(id)

    @staticmethod
    def dump_mwm():
        parser = argparse.ArgumentParser(description="Dumps some MWM structures.")
        parser.add_argument("--path", type=str, required=True, help="Path to mwm.")
        parser.add_argument(
            "--format",
            type=str,
            default="str",
            choices=("str", "json"),
            help="Output format.",
        )
        parser.add_argument(
            "--need_features", action="store_true", help="Need to dump features."
        )
        args = parser.parse_args(sys.argv[2:])
        dump_mwm(args.path, args.format, args.need_features)

    @staticmethod
    def find_feature():
        parser = argparse.ArgumentParser(
            description="Finds features in an mwm file based on a query."
        )
        parser.add_argument("--path", type=str, required=True, help="Path to mwm.")
        parser.add_argument(
            "--type",
            type=str,
            required=True,
            choices=["t", "et", "n", "m", "id"],
            help="""Type:
  t for inside types ("t hwtag" will find all hwtags-*)
  et for exact type ("et shop" won\'t find shop-chemist)
  n for names, case-sensitive ("n Starbucks" fo r all starbucks)
  m for metadata keys ("m flats" for features with flats
  id for feature id ("id 1234" for feature #1234""",
        )
        parser.add_argument(
            "--str", type=str, required=True, help="String to find in mwm"
        )
        args = parser.parse_args(sys.argv[2:])
        find_and_print_features(args.path, args.type, args.str)

    @staticmethod
    def ft2osm():
        parser = argparse.ArgumentParser(
            description="Finds features in an mwm file based on a query."
        )
        parser.add_argument(
            "--path", type=str, required=True, help="Path to osm to feature mapping."
        )
        parser.add_argument("--id", type=str, required=True, help="Feature id.")
        args = parser.parse_args(sys.argv[2:])
        id = ft2osm(args.path, args.id)
        if id is None:
            print("Error: id not found.")
            exit(1)
        print(id)

    @staticmethod
    def mwm_feature_compare():
        parser = argparse.ArgumentParser(
            description="Compares feature count in .mwm files."
        )
        parser.add_argument(
            "-n", "--new", help="New mwm files path", type=str, required=True
        )
        parser.add_argument(
            "-o", "--old", help="Old mwm files path", type=str, required=True
        )
        parser.add_argument(
            "-f", "--feature", help="Feature name to count", type=str, required=True
        )
        parser.add_argument(
            "-t",
            "--threshold",
            help="Threshold in percent to warn",
            type=int,
            default=20,
        )

        args = parser.parse_args(sys.argv[2:])
        compare_mwm(args.old, args.new, args.feature, args.threshold)


Mwm()
