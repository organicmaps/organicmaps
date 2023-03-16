#!/usr/bin/env python3
"""Parses GTFS feeds urls:
https://transit.land/        - Transitland
https://storage.googleapis.com/storage/v1/b/mdb-csv/o/sources.csv?alt=media  
                            - Mobility Database (https://mobilitydata.org/)  
Crawls all the urls, loads feed zips and extracts to the specified directory."""

import argparse
import concurrent.futures
import io
import json
import logging
import os
import csv
import time
import zipfile

import requests

MAX_RETRIES = 2
MAX_SLEEP_TIMEOUT_S = 30

RAW_FILE_MOBILITYDB = "raw_mobilitydb.csv"

URLS_FILE_TRANSITLAND = "feed_urls_transitland.txt"
URLS_FILE_MOBILITYDB = "feed_urls_mobilitydb.txt"

URL_MOBILITYDB_GTFS_SOURCE = "https://storage.googleapis.com/storage/v1/b/mdb-csv/o/sources.csv?alt=media"

THREADS_COUNT = 2
MAX_INDEX_LEN = 4


logger = logging.getLogger(__name__)


def download_gtfs_sources_mobilitydb(path):
    """Downloads the csv catalogue from Data Mobility"""
    try:
        req = requests.get(URL_MOBILITYDB_GTFS_SOURCE)
        url_content = req.content
        with open(os.path.join(path, RAW_FILE_MOBILITYDB), 'wb') as csv_file:
            csv_file.write(url_content)
    except requests.exceptions.HTTPError as http_err:
        logger.error(
            f"HTTP error {http_err} downloading zip from {URL_MOBILITYDB_GTFS_SOURCE}")


def get_gtfs_urls_mobilitydb(path, countries_list):
    """Extracts the feed urls from the downloaded csv file"""
    download_from_all_countries = True
    if countries_list:
        download_from_all_countries = False

    download_gtfs_sources_mobilitydb(path)
    file = open(os.path.join(path, RAW_FILE_MOBILITYDB), encoding='UTF-8')
    raw_sources = csv.DictReader(file)
    next(raw_sources)
    urls = [field["urls.direct_download"] for field in raw_sources if download_from_all_countries or field["location.country_code"] in countries_list]
    write_list_to_file(os.path.join(path, URLS_FILE_MOBILITYDB), urls)


def get_feeds_links(data):
    """Extracts feed urls from the GTFS json description."""
    gtfs_feeds_urls = []

    for feed in data:
        # Possible values: MDS, GBFS, GTFS_RT, GRFS
        if feed["spec"].lower() != "gtfs":
            continue

        if "urls" in feed and feed["urls"] is not None and feed["urls"]:
            gtfs_feeds_urls.append(feed["urls"]["static_current"])

    return gtfs_feeds_urls


def parse_transitland_page(url):
    """Parses page with feeds list, extracts feeds urls and the next page url."""
    retries = MAX_RETRIES

    while retries > 0:
        try:
            response = requests.get(url)
            response.raise_for_status()

            data = json.loads(response.text)
            if "feeds" in data:
                gtfs_feeds_urls = get_feeds_links(data["feeds"])
            else:
                gtfs_feeds_urls = []

            next_page = data["meta"]["next"] if "next" in data.get("meta", "") else ""
            return gtfs_feeds_urls, next_page

        except requests.exceptions.HTTPError as http_err:
            logger.error(f"HTTP error {http_err} downloading zip from {url}")
            if http_err == 429:
                time.sleep(MAX_SLEEP_TIMEOUT_S)
        except requests.exceptions.RequestException as ex:
            logger.error(
                f"Exception {ex} while parsing Transitland url {url} with code {response.status_code}"
            )

        retries -= 1

    return [], ""


def extract_to_path(content, out_path):
    """Reads content as zip and extracts it to out_path."""
    try:
        archive = zipfile.ZipFile(io.BytesIO(content))
        archive.extractall(path=out_path)
        return True
    except zipfile.BadZipfile:
        logger.exception("BadZipfile exception.")
    except Exception as e:
        logger.exception(f"Exception while unzipping feed: {e}")

    return False


def load_gtfs_feed_zip(path, url):
    """Downloads url-located zip and extracts it to path/index."""
    retries = MAX_RETRIES
    while retries > 0:
        try:
            response = requests.get(url, stream=True)
            response.raise_for_status()

            if not extract_to_path(response.content, path):
                retries -= 1
                logger.error(f"Could not extract zip: {url}")
                continue

            return True

        except requests.exceptions.HTTPError as http_err:
            logger.error(f"HTTP error {http_err} downloading zip from {url}")
        except requests.exceptions.RequestException as ex:
            logger.error(f"Exception {ex} downloading zip from {url}")
        retries -= 1

    return False


def write_list_to_file(path, lines):
    """Saves list of lines to path."""
    with open(path, "w") as out:
        out.write("\n".join(lines))


def crawl_transitland_for_feed_urls(out_path, transitland_api_key):
    """Crawls transitland feeds API and parses feeds urls from json on each page
    Do not try to parallel it because of the Transitland HTTP requests restriction."""
    start_page = "https://transit.land/api/v2/rest/feeds?api_key={}".format(transitland_api_key)

    total_feeds = []
    gtfs_feeds_urls, next_page = parse_transitland_page(start_page)

    while next_page:
        logger.info(f"Loaded {next_page}")
        total_feeds += gtfs_feeds_urls
        gtfs_feeds_urls, next_page = parse_transitland_page(next_page)

    if gtfs_feeds_urls:
        total_feeds += gtfs_feeds_urls

    write_list_to_file(os.path.join(out_path, URLS_FILE_TRANSITLAND), total_feeds)


def get_filename(file_prefix, index):
    return f"{file_prefix}_{index:0{MAX_INDEX_LEN}d}"


def load_gtfs_zips_from_urls(path, urls_file, threads_count, file_prefix):
    """Concurrently downloads feeds zips from urls to path."""
    urls = [url.strip() for url in open(os.path.join(path, urls_file))]
    if not urls:
        logger.error(f"Empty urls from {path}")
        return
    logger.info(f"Preparing to load feeds: {len(urls)}")
    err_count = 0

    with concurrent.futures.ThreadPoolExecutor(max_workers=threads_count) as executor:
        future_to_url = {
            executor.submit(
                load_gtfs_feed_zip,
                os.path.join(path, get_filename(file_prefix, i)),
                url,
            ): url
            for i, url in enumerate(urls)
        }

        for j, future in enumerate(
            concurrent.futures.as_completed(future_to_url), start=1
        ):
            url = future_to_url[future]

            loaded = future.result()
            if not loaded:
                err_count += 1
            logger.info(f"Handled {j}/{len(urls)} feed. Loaded = {loaded}. {url}")

    logger.info(f"Done loading. {err_count}/{len(urls)} errors")



def main():
    """Downloads urls of feeds from feed aggregators and saves to the file.
    Downloads feeds from these urls and saves to the directory."""

    parser = argparse.ArgumentParser(
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument("-p", "--path", required=True, help="working directory path")

    parser.add_argument(
        "-m", "--mode", required=True, help="fullrun | load_feed_urls | load_feed_zips"
    )

    parser.add_argument(
        "-s",
        "--source",
        default="transitland",
        help="source of feeds: transitland | mobilitydb | all",
    )

    parser.add_argument(
        "-t",
        "--threads",
        type=int,
        default=THREADS_COUNT,
        help="threads count for loading zips",
    )

    # Required in order to use Transitlands api
    parser.add_argument(
        "-T",
        "--transitland_api_key",
        help="user key for working with transitland API v2"
    )

    # Example: to download data only for Germany and France use "--mdb_countries DE,FR"
    parser.add_argument(
        "-c",
        "--mdb_countries",
        help="use data from MobilityDatabase only from selected countries (use ISO codes)",
    )

    args = parser.parse_args()

    logging.basicConfig(
        filename=os.path.join(args.path, "crawling.log"),
        filemode="w",
        level=logging.INFO,
    )

    if args.mode in ["fullrun", "load_feed_urls"]:

        if args.source in ["all", "mobilitydb"]:
            mdb_countries = []
            if args.mdb_countries:
                mdb_countries = args.mdb_countries.split(',')

            get_gtfs_urls_mobilitydb(args.path, mdb_countries)
        if args.source in ["all", "transitland"]:
            if not args.transitland_api_key:
                logger.error(
                    "No key provided for Transit Land. Set transitland_api_key argument."
                )
                return
            crawl_transitland_for_feed_urls(args.path, args.transitland_api_key)

    if args.mode in ["fullrun", "load_feed_zips"]:

        if args.source in ["all", "transitland"]:
            load_gtfs_zips_from_urls(
                args.path, URLS_FILE_TRANSITLAND, args.threads, "tl"
            )
        if args.source in ["all", "mobilitydb"]:
            load_gtfs_zips_from_urls(args.path, URLS_FILE_MOBILITYDB, args.threads, "mdb")


if __name__ == "__main__":
    main()
