"""
Embed public holidays directly into data/countries_meta.txt

Usage:
    python3 countries_meta_holidays.py
    python3 countries_meta_holidays.py --year 2025
    python3 countries_meta_holidays.py --country Spain
"""

import json
import pycountry
import holidays
import os
import argparse
import datetime
import re
import sys

custom_name_to_iso = {
    "Saint Barthelemy": "BL",
    "United States Virgin Islands": "VI",
    "Brunei": "BN",
    "Cape Verde": "CV",
    "Congo-Brazzaville": "CG",
    "Congo-Kinshasa": "CD",
    "Czech": "CZ",
    "Cote dIvoire": "CI",
    "East Timor": "TL",
    "Falkland Islands": "FK",
    "Macedonia": "MK",
    "Pitcairn Islands": "PN",
    "Saint Helena Ascension and Tristan da Cunha": "SH",
    "Republic of Kosovo": "XK",
    "Russia": "RU",
    "Palestine": "PS",
    "Swaziland": "SZ",
    "The Bahamas": "BS",
    "The Gambia": "GM",
    "Turkey": "TR",
    "UK": "GB",
    "New Zealand South": "NZ",
    "New Zealand North": "NZ",
    "Abkhazia": "GE",
    "South Ossetia": "GE",
    "Jerusalem": "IL",
    "Crimea": "RU",
    "Campo de Hielo Sur": "AR",
    "Caribisch Nederland": "BQ",
    "People’s Republic of China": "CN",
    "Sahrawi Arab Democratic Republic": "EH",
    "Kingdom of Lesotho": "LS",
    "Willis Island": "AU",
    "Saint Martin": "MF",
}

UNWANTED_PHRASES = [
    "observed",
    "in lieu",
    "by old style",
    "by new style",
    "non-working",
    "substituted from",
    "National",
    "substitute day",
]

# Paths
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, "../.."))
meta_path = os.path.join(project_root, "data", "countries_meta.txt")


def parse_args():
    parser = argparse.ArgumentParser(
        description="Embed public holidays into data/countries_meta.txt",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("--year", type=int, default=datetime.datetime.now().year)
    parser.add_argument("--country", type=str, default=None)
    return parser.parse_args()


def clean_holiday_name(name: str) -> str:
    for phrase in UNWANTED_PHRASES:
        name = name.replace(f" ({phrase})", "")
        name = name.replace(f" ({phrase.title()})", "")
    name = re.sub(r"\s*\(substituted from [^)]+\)", "", name, flags=re.IGNORECASE)
    return name.strip()


def get_iso_code(base_name: str):
    try:
        country_obj = pycountry.countries.lookup(base_name)
        return country_obj.alpha_2
    except LookupError:
        return custom_name_to_iso.get(base_name)


def write_value_like_original(f, value, level: int):
    if isinstance(value, dict):
        f.write("{\n")
        items = list(value.items())
        for i, (k, v) in enumerate(items):
            f.write(" " * (level + 1))
            f.write(json.dumps(k, ensure_ascii=False))
            f.write(": ")
            write_value_like_original(f, v, level + 1)
            if i < len(items) - 1:
                f.write(",\n")
        f.write("\n" + " " * level + "}")
    elif isinstance(value, list):
        f.write("[")
        for i, item in enumerate(value):
            f.write(json.dumps(item, ensure_ascii=False))
            if i < len(value) - 1:
                f.write(", ")
        f.write("]")
    else:
        f.write(json.dumps(value, ensure_ascii=False))


def write_meta_like_original(path: str, meta: dict):
    with open(path, "w", encoding="utf-8") as f:
        f.write("{\n")
        items = list(meta.items())
        for i, (k, v) in enumerate(items):
            f.write(json.dumps(k, ensure_ascii=False))
            f.write(": ")
            write_value_like_original(f, v, 0)
            if i < len(items) - 1:
                f.write(",\n")
        f.write("\n}\n")


def main():
    args = parse_args()
    year = args.year
    country_filter = args.country

    with open(meta_path, "r", encoding="utf-8") as f:
        meta = json.load(f)

    if country_filter and country_filter not in meta:
        print(f"Error: {country_filter} not found in countries_meta.txt")
        sys.exit(1)

    for map_name, data in meta.items():
        if country_filter and map_name != country_filter:
            continue

        if "_" in map_name:
            continue

        base_name = map_name
        iso_code = get_iso_code(base_name)

        if not iso_code:
            print(f"[skip] No ISO/holidays for {map_name}")
            continue

        try:
            country_holidays = holidays.country_holidays(
                iso_code,
                years=range(year, year + 2),
            )
        except Exception as e:
            print(f"[skip] Holidays not available for {map_name} ({iso_code}): {e}")
            continue

        holidays_dict = {
            str(date): clean_holiday_name(name)
            for date, name in country_holidays.items()
        }

        if holidays_dict:
            data["public_holidays"] = holidays_dict

    write_meta_like_original(meta_path, meta)
    print(f"Updated holidays embedded into {meta_path}")


if __name__ == "__main__":
    main()
