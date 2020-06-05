import argparse
import itertools
import logging
import os

import wikipediaapi

from descriptions.descriptions_downloader import check_and_get_checker
from descriptions.descriptions_downloader import download_from_wikidata_tags
from descriptions.descriptions_downloader import download_from_wikipedia_tags
from descriptions.descriptions_downloader import log


def parse_args():
    parser = argparse.ArgumentParser(description="Download wiki pages.")
    parser.add_argument(
        "--output_dir", metavar="PATH", type=str, help="Output dir for saving pages"
    )
    parser.add_argument(
        "--popularity",
        metavar="PATH",
        type=str,
        help="File with popular object ids for which we "
        "download wikipedia data. If not given, download "
        "for all objects.",
    )
    parser.add_argument(
        "--wikipedia",
        metavar="PATH",
        type=str,
        required=True,
        help="Input file with wikipedia url.",
    )
    parser.add_argument(
        "--wikidata", metavar="PATH", type=str, help="Input file with wikidata ids."
    )
    parser.add_argument(
        "--langs",
        metavar="LANGS",
        type=str,
        nargs="+",
        action="append",
        help="Languages ​​for pages. If left blank, pages in all "
        "available languages ​​will be loaded.",
    )
    return parser.parse_args()


def main():
    log.setLevel(logging.WARNING)
    wikipediaapi.log.setLevel(logging.WARNING)
    args = parse_args()
    wikipedia_file = args.wikipedia
    wikidata_file = args.wikidata
    output_dir = args.output_dir
    popularity_file = args.popularity
    langs = list(itertools.chain.from_iterable(args.langs))
    os.makedirs(output_dir, exist_ok=True)
    checker = check_and_get_checker(popularity_file)
    download_from_wikipedia_tags(wikipedia_file, output_dir, langs, checker)
    if wikidata_file is None:
        log.warning(f"Wikidata file not set.")
    elif os.path.exists(wikidata_file):
        download_from_wikidata_tags(wikidata_file, output_dir, langs, checker)
    else:
        log.warning(f"Wikidata ({wikidata_file}) file not set.")


main()
