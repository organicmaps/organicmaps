from abc import ABC, abstractmethod
from typing import Optional

import requests
import json
from datetime import date, timedelta

from .holiday import HolidayDb, HolidayType, CountryIsoCode
from .metadata import IsoCodes


class DataSource(ABC):
    _HolidayTypeMap = {t.name: t for t in HolidayType}

    def __init__(self, supported_countries: IsoCodes, duplicates_correction_map: dict, supported_languages: list[str],
                 holiday_db: HolidayDb,
                 holiday_types: list[HolidayType]):
        self.supported_countries = supported_countries
        self.duplicates_correction_map = duplicates_correction_map
        self.supported_languages = supported_languages
        self.holiday_db = holiday_db
        self.holiday_types = holiday_types

    @abstractmethod
    def load_holidays(self, iso_codes: IsoCodes, years: list[int]) -> None:
        pass


class OpenHolidaysApi(DataSource):
    _API = "https://openholidaysapi.org/PublicHolidays?countryIsoCode={country}&validFrom={date_start}&validTo={date_end}"

    def load_holidays(self, iso_codes: IsoCodes, years: list[int]) -> None:
        countries = self._prepare_iso_codes(iso_codes)

        for country in countries:
            for year in years:
                response = self._send_request(country, year)
                if not response:
                    continue

                self._parse_response(response, country, iso_codes)

    def _prepare_iso_codes(self, iso_codes: IsoCodes) -> IsoCodes:
        """
        OpenHolidaysApi "PublicHolidays" request only supports ISO 3166-1 alpha-2 codes
        """
        iso_codes = list(sorted(set(map(lambda code: code[:2] if len(code) > 2 else code, iso_codes))))
        return [code for code in iso_codes if code in self.supported_countries]

    @staticmethod
    def _send_request(country: CountryIsoCode, year: int) -> Optional[str]:
        date_start = f"{year}-01-01"
        date_end = f"{year}-12-31"
        url = OpenHolidaysApi._API.format(country=country, date_start=date_start, date_end=date_end)
        print(f"Sending request to {url}")
        try:
            with requests.request("GET", url) as resp:
                if resp.status_code != 200:
                    raise Exception(f"Failed to fetch {url}")
                return resp.text
        except Exception as e:
            print(f"Failed to fetch {url}: {e}")
            return None

    def _parse_response(self, response: str, country: CountryIsoCode, iso_codes: IsoCodes) -> None:
        entries = json.loads(response)
        for entry in entries:
            translations = {}
            for lang in entry["name"]:
                translations[lang["language"]] = lang["text"]
            if "EN" not in translations:
                print(f"[LANGUAGE] Skipping holiday \"{entry['name']}\" for {country} (no English translation)")
                continue
            name = translations["EN"]
            if "observed" in name.lower():
                print(f"[OBSERVED] Skipping holiday \"{name}\" for {country} (observed)")
                continue
            if name in self.duplicates_correction_map:
                print(f"[DUPLICATE] Correcting holiday name \"{name}\" to \"{self.duplicates_correction_map[name]}\"")
                name = self.duplicates_correction_map[name]

            holiday_type = DataSource._HolidayTypeMap.get(entry["type"], HolidayType.Optional)
            if holiday_type not in self.holiday_types:
                print(f"[TYPE] Skipping holiday \"{name}\" of type {holiday_type} for {country}")
                continue

            counties = [div["code"] for div in entry.get("subdivisions")] if entry.get("subdivisions") else [country]
            codes = []
            for code in counties:
                if code not in iso_codes:
                    print(f"[COUNTRY] Skipping holiday \"{name}\" for {code} ({country})")
                    continue
                codes.append(code)
            if not codes:
                continue

            date_start = date.fromisoformat(entry["startDate"])
            date_end = date.fromisoformat(entry["endDate"])

            holiday = self.holiday_db.get_holiday(name)
            if holiday.type is not None and holiday.type != holiday_type:
                print(f"[TYPE] Overwriting holiday type \"{holiday.type}\" with \"{holiday_type}\" for \"{name}\"")
            holiday.type = holiday_type

            for lang, translation in translations.items():
                if lang == "EN":
                    continue
                lang_key = lang.lower()
                if lang_key not in self.supported_languages:
                    print(f"[LANGUAGE] Unsupported language \"{lang}\" for \"{name}\" ({lang_key})")
                    continue
                if lang_key in holiday.name.translations:
                    print(f"[LANGUAGE] Ignoring duplicate translation for \"{name}\" ({lang_key})")
                    continue
                holiday.name.translations[lang.lower()] = translation

            for code in codes:
                d = date_start
                while d <= date_end:
                    holiday.add_date(code, d)
                    d += timedelta(days=1)


class NagerDataSource(DataSource):
    _API = "https://date.nager.at/api/v3/publicholidays/{year}/{country}"

    def load_holidays(self, iso_codes: IsoCodes, years: list[int]) -> None:
        countries = self._prepare_iso_codes(iso_codes)

        for country in countries:
            for year in years:
                response = self._send_request(country, year)
                if not response:
                    continue

                self._parse_response(response, country, iso_codes)

    def _prepare_iso_codes(self, iso_codes: IsoCodes) -> IsoCodes:
        """
        Nager API only supports ISO 3166-1 alpha-2 codes
        """
        iso_codes = list(sorted(set(map(lambda code: code[:2] if len(code) > 2 else code, iso_codes))))
        return [code for code in iso_codes if code in self.supported_countries]

    @staticmethod
    def _send_request(country: CountryIsoCode, year: int) -> Optional[str]:
        url = NagerDataSource._API.format(year=year, country=country)
        print(f"Sending request to {url}")
        try:
            with requests.request("GET", url) as resp:
                if resp.status_code != 200:
                    raise Exception(f"Failed to fetch {url}")
                return resp.text
        except Exception as e:
            print(f"Failed to fetch {url}: {e}")
            return None

    def _parse_response(self, response: str, country: CountryIsoCode, iso_codes: IsoCodes) -> None:
        entries = json.loads(response)
        for entry in entries:
            name: str = entry["name"]
            if name in self.duplicates_correction_map:
                print(f"[DUPLICATE] Correcting holiday name \"{name}\" to \"{self.duplicates_correction_map[name]}\"")
                name = self.duplicates_correction_map[name]

            raw_type: str = (entry.get("types") or ["Public"])[0]
            holiday_type = DataSource._HolidayTypeMap.get(raw_type, HolidayType.Optional)
            if holiday_type not in self.holiday_types:
                print(f"[TYPE] Skipping holiday \"{name}\" of type {holiday_type} for {country}")
                continue

            # Determine which iso codes this entry applies to:
            # counties lists ISO 3166-2 subdivisions; if absent/null it's national
            counties = entry.get("counties") if entry.get("counties") else [country]
            codes = []
            for code in counties:
                if code not in iso_codes:
                    print(f"[COUNTRY] Skipping holiday \"{name}\" for {code} ({country})")
                    continue
                codes.append(code)
            if not codes:
                continue

            d = date.fromisoformat(entry["date"])

            holiday = self.holiday_db.get_holiday(name)
            if holiday.type is not None and holiday.type != holiday_type:
                print(f"[TYPE] Overwriting holiday type \"{holiday.type}\" with \"{holiday_type}\" for \"{name}\"")
            holiday.type = holiday_type
            for code in codes:
                holiday.add_date(code, d)


def create_data_source(name: str, supported_countries: IsoCodes, duplicates_correction_map: dict,
                       supported_languages: list[str], holiday_db: HolidayDb,
                       holiday_types: list[HolidayType]) -> DataSource:
    if name == "Nager.Date":
        return NagerDataSource(supported_countries, duplicates_correction_map, supported_languages, holiday_db,
                               holiday_types)
    if name == "OpenHolidays.Api":
        return OpenHolidaysApi(supported_countries, duplicates_correction_map, supported_languages, holiday_db,
                               holiday_types)
    raise Exception(f"Unknown data source: {name}")
