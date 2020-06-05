import argparse
import datetime
import logging
import os
import sys

from tqdm import tqdm

from booking.api.booking_api import LIMIT_REQUESTS_PER_MINUTE
from booking.download_hotels import download
from booking.download_test_data import download_test_data


def process_options():
    parser = argparse.ArgumentParser(description="Download and process booking hotels.")
    parser.add_argument("-v", "--verbose", action="store_true")
    parser.add_argument(
        "--logfile", default="", help="Name and destination for log file"
    )
    parser.add_argument(
        "--password",
        required=True,
        dest="password",
        help="Booking.com account password",
    )
    parser.add_argument(
        "--user", required=True, dest="user", help="Booking.com account user name"
    )
    parser.add_argument(
        "--threads_count",
        default=1,
        type=int,
        help="The number of threads for processing countries.",
    )
    parser.add_argument(
        "--output",
        required=True,
        dest="output",
        help="Name and destination for output file",
    )
    parser.add_argument(
        "--country_code",
        default=None,
        action="append",
        help="Download hotels of this country.",
    )
    parser.add_argument(
        "--download_test_dataset", default=False, help="Download dataset for tests."
    )
    options = parser.parse_args()
    return options


def main():
    options = process_options()
    logfile = ""
    if options.logfile:
        logfile = options.logfile
    else:
        now = datetime.datetime.now()
        name = f"{now.strftime('%d_%m_%Y-%H_%M_%S')}_booking_hotels.log"
        logfile = os.path.join(os.path.dirname(os.path.realpath(__file__)), name)
    print(f"Logs saved to {logfile}.", file=sys.stdout)
    if options.threads_count > 1:
        print(
            f"Limit requests per minute is {LIMIT_REQUESTS_PER_MINUTE}.",
            file=sys.stdout,
        )
    logging.basicConfig(
        level=logging.DEBUG,
        filename=logfile,
        format="%(thread)d [%(asctime)s] %(levelname)s: %(message)s",
    )
    with tqdm(disable=not options.verbose) as progress_bar:
        if options.download_test_dataset:
            download_test_data(
                options.country_code,
                options.user,
                options.password,
                options.output,
                options.threads_count,
                progress_bar,
            )
        else:
            download(
                options.country_code,
                options.user,
                options.password,
                options.output,
                options.threads_count,
                progress_bar,
            )


main()
