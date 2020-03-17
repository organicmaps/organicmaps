#!/usr/bin/env python

import argparse

from pygen import classif
from pygen import mwm


def main(args):
    map = mwm.Mwm(args.m)
    print("{} with type {} has {} features.".format(args.m, map.type(), len(map)))
    print("Mwm version is {}".format(map.version()))
    print("Bounds are {}".format(map.bounds()))
    print("Sections info is {}".format(map.sections_info()))

    fts_with_metadata = [ft for ft in map if ft.metadata()]
    print("{} features have metadata.".format(len(fts_with_metadata)))

    first = fts_with_metadata[0]
    print(first)

    metadata = first.metadata()
    if mwm.EType.FMD_CUISINE in metadata:
        print("Metadata has mwm.EType.FMD_CUISINE.")

    if mwm.EType.FMD_WEBSITE in metadata:
        print("Metadata has mwm.EType.FMD_WEBSITE.")

    for k, v in metadata.items():
        print("{}: {}".format(k, v))

    last = fts_with_metadata[-1]
    print(last.metadata())
    for t in last.types():
        print(classif.readable_type(t))

    print("Index is {}".format(last.index()))
    print("Center is {}".format(last.center()))
    print(
        "Names are {}, readable name is {}.".format(last.names(), last.readable_name())
    )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Example usage of pygen module.")
    parser.add_argument(
        "-m", metavar="MWM_PATH", type=str, default="", help="Path to mwm files."
    )
    args = parser.parse_args()
    main(args)
