#!/usr/bin/env python
# coding: utf8
import argparse
import datetime
import logging
import os
import statistics
import sys
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from functools import partial
from multiprocessing.pool import ThreadPool
from random import randint
from threading import Event
from time import sleep

import eviltransform
import math
import requests
from ratelimit import limits, sleep_and_retry
from tqdm import tqdm

LIMIT_REQUESTS_PER_MINUTE = 400
ATTEMPTS_COUNT = 10
MINMAX_LIMIT_WAIT_AFTER_429_ERROR_SECONDS = (30, 120)
SUPPORTED_LANGUAGES = ("en", "ru", "ar", "cs", "da", "nl", "fi", "fr", "de",
                       "hu", "id", "it", "ja", "ko", "pl", "pt", "ro", "es",
                       "sv", "th", "tr", "uk", "vi", "zh", "he", "sk", "el")


class AppError(Exception):
    pass


class HTTPError(AppError):
    pass


class AttemptsSpentError(AppError):
    pass


class GettingMinPriceError(AppError):
    pass


class BookingApi:
    ENDPOINTS = {
        "countries": "list",
        "hotels": "list"
    }

    def __init__(self, login, password, version):
        major_minor = version.split(".")
        assert len(major_minor) == 2
        assert int(major_minor[0]) >= 2
        assert 0 <= int(major_minor[1]) <= 4

        self._event = Event()
        self._event.set()
        self._timeout = 5 * 60  # in seconds
        self._login = login
        self._password = password
        self._base_url = f"https://distribution-xml.booking.com/{version}/json"
        self._set_endpoints()

    @sleep_and_retry
    @limits(calls=LIMIT_REQUESTS_PER_MINUTE, period=60)
    def call_endpoint(self, endpoint, **params):
        self._event.wait()
        try:
            attempts = ATTEMPTS_COUNT
            while attempts:
                attempts -= 1
                response = None
                try:
                    response = requests.get(f"{self._base_url}/{endpoint}",
                                            auth=(self._login, self._password),
                                            params=params, timeout=self._timeout)
                except requests.exceptions.ReadTimeout:
                    logging.exception("Timeout error.")
                    continue
                if response.status_code == 200:
                    data = response.json()
                    return data["result"]
                else:
                    self._handle_errors(response)
            raise AttemptsSpentError(f"{ATTEMPTS_COUNT} attempts were spent.")
        except Exception as e:
            if not self._event.is_set():
                self._event.set()
            raise e

    def _handle_errors(self, response):
        error_message = ""
        data = response.json()
        try:
            error_message = ",".join(x["message"] for x in data["errors"])
        except KeyError:
            error_message = data

        if response.status_code == 429:
            self._event.clear()
            wait_seconds = randint(*MINMAX_LIMIT_WAIT_AFTER_429_ERROR_SECONDS)
            logging.warning(f"Http error {response.status_code}: {error_message}. "
                            f"It waits {wait_seconds} seconds and tries again.")
            sleep(wait_seconds)
            self._event.set()
        else:
            raise HTTPError(
                f"Http error with code {response.status_code}: {error_message}.")

    def _set_endpoints(self):
        for endpoint in BookingApi.ENDPOINTS:
            setattr(self, endpoint, partial(self.call_endpoint, endpoint))


class BookingListApi:
    _ROWS_BY_REQUEST = 1000

    def __init__(self, api):
        self.api = api
        self._set_endpoints()

    def call_endpoint(self, endpoint, **params):
        result = []
        offset = 0
        while True:
            resp = self._call_endpoint_offset(offset, endpoint, **params)
            result.extend(resp)
            if len(resp) < BookingListApi._ROWS_BY_REQUEST:
                break
            offset += BookingListApi._ROWS_BY_REQUEST
        return result

    def _call_endpoint_offset(self, offset, endpoint, **params):
        r = self.api.call_endpoint(endpoint, **{
            "offset": offset,
            "rows": BookingListApi._ROWS_BY_REQUEST,
            **params
        })
        if not isinstance(r, list):
            raise TypeError(f"Result has unexpected type {type(r)}")
        return r

    def _set_endpoints(self):
        for endpoint in BookingApi.ENDPOINTS:
            if BookingApi.ENDPOINTS[endpoint] == "list":
                setattr(self, endpoint, partial(self.call_endpoint, endpoint))


class BookingGen:
    def __init__(self, api, country):
        self.api = api
        self.country_code = country["country"]
        self.country_name = country["name"]
        logging.info(f"Download[{self.country_code}]: {self.country_name}")

        extras = ["hotel_info", "room_info"]
        self.hotels = self._download_hotels(extras=extras)
        self.translations = self._download_translations()
        self.currency_medians = self._currency_medians_by_cities()

    def generate_tsv_rows(self, sep="\t"):
        self._fix_hotels()
        return (self._create_tsv_hotel_line(hotel, sep) for hotel in self.hotels)

    @staticmethod
    def _get_hotel_min_price(hotel):
        prices = (float(x["room_info"]["min_price"]) for x in hotel["room_data"])
        flt = filter(lambda x: not math.isclose(x, 0.0), prices)
        try:
            return min(flt)
        except ValueError:
            raise GettingMinPriceError(f"Getting min price error: {prices}.")

    @staticmethod
    def _format_string(s):
        s = s.strip()
        for x in (("\t", " "), ("\n", " "), ("\r", "")):
            s = s.replace(*x)
        return s

    def _download_hotels(self, **params):
        return self.api.hotels(country_ids=self.country_code, **params)

    def _download_translations(self):
        extras = ["hotel_info", ]
        translations = defaultdict(dict)
        with ThreadPoolExecutor(max_workers=len(SUPPORTED_LANGUAGES)) as executor:
            m = {executor.submit(self._download_hotels, extras=extras, language=lang): lang
                 for lang in SUPPORTED_LANGUAGES}
            for future in as_completed(m):
                lang = m[future]
                hotels = future.result()
                for hotel in hotels:
                    hotel_id = hotel["hotel_id"]
                    hotel_data = hotel["hotel_data"]
                    translations[hotel_id][lang] = {
                        "name": BookingGen._format_string(hotel_data["name"]),
                        "address": BookingGen._format_string(hotel_data["address"])
                    }
        return translations

    def _fix_hotels(self):
        if self.country_code == "cn":
            # Fix chinese coordinates.
            # https://en.wikipedia.org/wiki/Restrictions_on_geographic_data_in_China
            for hotel in self.hotels:
                hotel_data = hotel["hotel_data"]
                location = hotel_data["location"]
                try:
                    location["latitude"], location["longitude"] = eviltransform.gcj2wgs_exact(
                        float(location["latitude"]), float(location["longitude"])
                    )
                except ValueError:
                    logging.exception(f"Converting error {location}")

    def _currency_medians_by_cities(self):
        cities = defaultdict(lambda: defaultdict(list))
        for hotel in self.hotels:
            hotel_data = hotel["hotel_data"]
            city_id = hotel_data["city_id"]
            currency = hotel_data["currency"]
            try:
                price = BookingGen._get_hotel_min_price(hotel)
            except GettingMinPriceError:
                logging.exception("Getting min price error.")
                continue
            cities[city_id][currency].append(price)

        for city in cities:
            for currency in cities[city]:
                cities[city][currency] = statistics.median(cities[city][currency])
        return cities

    def _get_rate(self, hotel):
        # Price rate ranges, relative to the median price for a city
        rates = (0.7, 1.3)
        rate = 0
        hotel_data = hotel["hotel_data"]
        city_id = hotel_data["city_id"]
        currency = hotel_data["currency"]
        price = None
        try:
            price = BookingGen._get_hotel_min_price(hotel)
        except GettingMinPriceError:
            logging.exception("Getting min price error.")
            return rate
        avg = self.currency_medians[city_id][currency]
        rate = 1
        # Find a range that contains the price
        while rate <= len(rates) and price > avg * rates[rate - 1]:
            rate += 1
        return rate

    def _get_translations(self, hotel):
        try:
            tr = self.translations[hotel["hotel_id"]]
        except KeyError:
            return ""

        hotel_data = hotel["hotel_data"]
        name = hotel_data["name"]
        address = hotel_data["address"]
        tr_ = defaultdict(dict)
        for k, v in tr.items():
            n = v["name"] if v["name"] != name else ""
            a = v["address"] if v["address"] != address else ""
            if a or n:
                tr_[k]["name"] = n
                tr_[k]["address"] = a

        tr_list = []
        for tr_lang, tr_values in tr_.items():
            tr_list.append(tr_lang)
            tr_list.extend([tr_values[e] for e in ("name", "address")])
        return "|".join(s.replace("|", ";") for s in tr_list)

    def _create_tsv_hotel_line(self, hotel, sep="\t"):
        hotel_data = hotel["hotel_data"]
        location = hotel_data["location"]
        row = (
            hotel["hotel_id"],
            f"{location['latitude']:.6f}",
            f"{location['longitude']:.6f}",
            hotel_data["name"],
            hotel_data["address"],
            hotel_data["class"],
            self._get_rate(hotel),
            hotel_data["ranking"],
            hotel_data["review_score"],
            hotel_data["url"],
            hotel_data["hotel_type_id"],
            self._get_translations(hotel)
        )
        return sep.join(BookingGen._format_string(str(x)) for x in row)


def download_hotels_by_country(api, country):
    generator = BookingGen(api, country)
    rows = list(generator.generate_tsv_rows())
    logging.info(f"For {country['name']} {len(rows)} lines were generated.")
    return rows


def download(country_code, user, password, path, threads_count, progress_bar):
    api = BookingApi(user, password, "2.4")
    list_api = BookingListApi(api)
    countries = list_api.countries(languages="en")
    if country_code is not None:
        countries = list(filter(lambda x: x["country"] in country_code, countries))
    logging.info(f"There is {len(countries)} countries.")
    progress_bar.total = len(countries)
    with open(path, "w") as f:
        with ThreadPool(threads_count) as pool:
            for lines in pool.imap_unordered(partial(download_hotels_by_country, list_api),
                                             countries):
                f.writelines([f"{x}\n" for x in lines])
                progress_bar.update()
    logging.info(f"Hotels were saved to {path}.")


def process_options():
    parser = argparse.ArgumentParser(description="Download and process booking hotels.")
    parser.add_argument("-q", "--quiet", action="store_false", dest="verbose")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose")
    parser.add_argument("--logfile", default="",
                        help="Name and destination for log file")
    parser.add_argument("--password", required=True, dest="password",
                        help="Booking.com account password")
    parser.add_argument("--user", required=True, dest="user",
                        help="Booking.com account user name")
    parser.add_argument("--threads_count", default=1, type=int,
                        help="The number of threads for processing countries.")
    parser.add_argument("--output", required=True, dest="output",
                        help="Name and destination for output file")
    parser.add_argument("--country_code", default=None, action="append",
                        help="Download hotels of this country.")
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
        print(f"Limit requests per minute is {LIMIT_REQUESTS_PER_MINUTE}.", file=sys.stdout)
    logging.basicConfig(level=logging.DEBUG, filename=logfile,
                        format="%(thread)d [%(asctime)s] %(levelname)s: %(message)s")
    with tqdm(disable=not options.verbose) as progress_bar:
        download(options.country_code, options.user, options.password,
                 options.output, options.threads_count, progress_bar)


if __name__ == "__main__":
    main()
