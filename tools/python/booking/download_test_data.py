import logging
import statistics
from functools import partial
from multiprocessing.pool import ThreadPool

import math
from tqdm import tqdm

from .api.booking_api import BookingApi, BookingListApi

SUPPORTED_LANGUAGES = ("en", "ru")


class BookingGen:
    def __init__(self, api, country, districtNames):
        self.api = api
        self.country_code = country["country"]
        self.country_name = country["name"]
        self.districtNames = districtNames
        logging.info(f"Download[{self.country_code}]: {self.country_name}")

        extras = ["hotel_info"]
        self.hotels = self._download_hotels(extras=extras)

    def generate_tsv_rows(self, sep="\t"):
        return (self._create_tsv_hotel_line(hotel, sep) for hotel in self.hotels)

    @staticmethod
    def _format_string(s):
        s = s.strip()
        for x in (("\t", " "), ("\n", " "), ("\r", "")):
            s = s.replace(*x)
        return s

    def _download_hotels(self, **params):
        return self.api.hotels(country_ids=self.country_code, **params)

    def _create_tsv_hotel_line(self, hotel, sep="\t"):
        hotel_data = hotel["hotel_data"]
        location = hotel_data["location"]
        district = "None"
        if hotel_data["district_id"] in self.districtNames:
          district = self.districtNames[hotel_data["district_id"]]
        row = (
            hotel["hotel_id"],
            hotel_data["address"],
            hotel_data["zip"],
            hotel_data["city"],
            district,
            self.country_name,
        )
        return sep.join(BookingGen._format_string(str(x)) for x in row)


def download_hotels_by_country(api, districtNames, country):
    generator = BookingGen(api, country, districtNames)
    rows = list(generator.generate_tsv_rows())
    logging.info(f"For {country['name']} {len(rows)} lines were generated.")
    return rows


def download_test_data(country_code, user, password, path, threads_count,
                       progress_bar=tqdm(disable=True)):
    logging.info(f"Starting test dataset download.")
    api = BookingApi(user, password, "2.4")
    list_api = BookingListApi(api)
    districts = list_api.districts(languages="en")
    districtNames = {}
    for district in districts:
      for translation in district['translations']:
         if translation['language'] == 'en':
           districtNames[district['district_id']] = translation['name']
    countries = list_api.countries(languages="en")
    if country_code is not None:
        countries = list(filter(lambda x: x["country"] in country_code, countries))
    logging.info(f"There are {len(countries)} countries.")
    progress_bar.desc = "Countries"
    progress_bar.total = len(countries)
    with open(path, "w") as f:
        with ThreadPool(threads_count) as pool:
            for lines in pool.imap_unordered(partial(download_hotels_by_country, list_api, districtNames),
                                             countries):
                f.writelines([f"{x}\n" for x in lines])
                progress_bar.update()
    logging.info(f"Hotels test dataset saved to {path}.")
