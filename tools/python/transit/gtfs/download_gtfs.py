"""Parses GTFS feeds urls:
https://transit.land/          - Transitland
http://transitfeeds.com/feeds  - Openmobilitydata
Crawls all the urls, loads feed zips and extracts to the specified directory."""

import argparse
import concurrent.futures
import io
import json
import logging
import os
import time
import zipfile

import requests

MAX_RETRIES = 2
MAX_SLEEP_TIMEOUT_S = 30

URLS_FILE_TRANSITLAND = "feed_urls_transitland.txt"
URLS_FILE_OMD = "feed_urls_openmobilitydata.txt"

THREADS_COUNT = 2
MAX_INDEX_LEN = 4

HEADERS_OMD = {"Accept": "application/json"}

logger = logging.getLogger(__name__)


def get_feeds_links(data):
    """Extracts feed urls from the GTFS json description."""
    gtfs_feeds_urls = []

    for feed in data:
        if feed["feed_format"] != "gtfs" or feed["spec"] != "gtfs":
            continue

        if "url" in feed and feed["url"] is not None and feed["url"]:
            gtfs_feeds_urls.append(feed["url"])

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

            next_page = data["meta"]["next"] if "next" in data["meta"] else ""
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


def parse_openmobilitydata_pages(omd_api_key):
    url_page = "https://api.transitfeeds.com/v1/getFeeds"
    url_with_redirect = "https://api.transitfeeds.com/v1/getLatestFeedVersion"
    page = pages_count = 1
    urls = []

    while page <= pages_count:
        params = {
            "key": omd_api_key,
            "page": page,
            "location": "undefined",
            "descendants": 1,
            "limit": 100,
            "type": "gtfs",
        }

        try:
            with requests.get(url_page, params=params, headers=HEADERS_OMD) as response:
                if response.status_code != 200:
                    logger.error(
                        f"Http code {response.status_code} loading feed ids: {url_page}"
                    )
                    return [], ""

                data = json.loads(response.text)

                if page == 1:
                    pages_count = data["results"]["numPages"]
                    logger.info(
                        f"There are {pages_count} Openmobilitydata pages with feed urls."
                    )

                for feed in data["results"]["feeds"]:
                    params = {"key": omd_api_key, "feed": feed["id"]}
                    try:
                        with requests.get(
                            url_with_redirect,
                            params=params,
                            headers=HEADERS_OMD,
                            allow_redirects=True,
                        ) as response_redirect:
                            if response_redirect.history:
                                urls.append(response_redirect.url)
                            else:
                                logger.error(
                                    f"Could not get link to zip with feed {feed['id']} from {url_with_redirect}"
                                )
                    except requests.exceptions.RequestException as ex_redirect:
                        logger.error(
                            f"Exception {ex_redirect} while getting link to zip with "
                            f"feed {feed['id']} from {url_with_redirect}"
                        )
        except requests.exceptions.RequestException as ex:
            logger.error(
                f"Exception {ex} while getting {url_page} (page {page}) from Openmobilitydata."
            )

        logger.info(f"Crawled {page}/{pages_count} page of Openmobilitydata.")
        page += 1

    return urls


def write_list_to_file(path, lines):
    """Saves list of lines to path."""
    with open(path, "w") as out:
        out.write("\n".join(lines))


def crawl_transitland_for_feed_urls(out_path):
    """Crawls transitland feeds API and parses feeds urls from json on each page
    Do not try to parallel it because of the Transitland HTTP requests restriction."""
    start_page = "https://api.transit.land/api/v1/feeds/?per_page=50"

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


def crawl_openmobilitydata_for_feed_urls(path, omd_api_key):
    """Crawls openmobilitydata feeds API and parses feeds urls from json on each page
    Do not try to parallel it because of the OpenMobilityData HTTP requests restriction."""
    feed_urls = parse_openmobilitydata_pages(omd_api_key)
    logger.info(f"Loaded feed urls {len(feed_urls)}")
    write_list_to_file(os.path.join(path, URLS_FILE_OMD), feed_urls)


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
        help="source of feeds: transitland | openmobilitydata | all",
    )

    parser.add_argument(
        "-t",
        "--threads",
        type=int,
        default=THREADS_COUNT,
        help="threads count for loading zips",
    )

    parser.add_argument(
        "-k",
        "--omd_api_key",
        default="",
        help="user key for working with openmobilitydata API",
    )

    args = parser.parse_args()

    logging.basicConfig(
        filename=os.path.join(args.path, "crawling.log"),
        filemode="w",
        level=logging.INFO,
    )

    if args.mode in ["fullrun", "load_feed_urls"]:

        if args.source in ["all", "transitland"]:
            crawl_transitland_for_feed_urls(args.path)
        if args.source in ["all", "openmobilitydata"]:
            if not args.omd_api_key:
                logger.error(
                    "No key provided for openmobilitydata. Set omd_api_key argument."
                )
                return
            crawl_openmobilitydata_for_feed_urls(args.path, args.omd_api_key)

    if args.mode in ["fullrun", "load_feed_zips"]:

        if args.source in ["all", "transitland"]:
            load_gtfs_zips_from_urls(
                args.path, URLS_FILE_TRANSITLAND, args.threads, "tl"
            )
        if args.source in ["all", "openmobilitydata"]:
            load_gtfs_zips_from_urls(args.path, URLS_FILE_OMD, args.threads, "omd")


if __name__ == "__main__":
    main()
