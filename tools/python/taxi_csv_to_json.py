#!/usr/bin/env python
# coding: utf8

from argparse import ArgumentParser
from collections import defaultdict

import json
import logging


def deserialize_places(src):
    lines = src.splitlines()
    # Skip header.
    lines = lines[1:]
    countries = defaultdict(list)
    mwms = []

    try:
        for l in lines:
            cells = l.split('\t')

            if len(cells) < 5 and not cells[0]:
                logging.error("Country cell is empty. Incorrect line: {}".format(cells))
                exit()

            # Add full country.
            if len(cells) < 3:
                countries[cells[0]] = []
            # Add city of the country.
            elif len(cells) < 5:
                countries[cells[0]].append(cells[2])
            # Add mwm.
            elif len(cells) >= 5:
                mwms.append(cells[4])
    except IndexError as e:
        logging.error("The structure of src file is incorrect. Exception: {}".format(e))
        exit()

    return countries, mwms


def convert(src_path, dst_path):
    try:
        with open(src_path, "r") as f:
            src = f.read()
    except (OSError, IOError):
        logging.error("Cannot read src file {}".format(src_path))
        return

    countries, mwms = deserialize_places(src)

    # Carcass of the result.
    result = {
        "enabled": {"countries": [], "mwms": []},
        "disabled": {"countries": [], "mwms": []}
    }

    for country, cities in countries.iteritems():
        result["enabled"]["countries"].append({
            "id": country,
            "cities": cities
        })

    result["enabled"]["mwms"] = mwms

    try:
        with open(dst_path, "w") as f:
            json.dump(result, f, indent=2, sort_keys=True)
    except (OSError, IOError):
        logging.error("Cannot write result into dst file {}".format(dst_path))
        return


def process_options():
    parser = ArgumentParser(description='Load taxi file in csv format and convert it into json')

    parser.add_argument("--src", type=str, dest="src", help="Path to csv file", required=True)
    parser.add_argument("--dst", type=str, dest="dst", help="Path to json file", required=True)

    options = parser.parse_args()

    if not options.src or not options.dst:
        parser.print_help()
        return None

    return options


def main():
    options = process_options()
    if options:
        convert(options.src, options.dst)


if __name__ == "__main__":
    main()
