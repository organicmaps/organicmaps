"""
Script to generate public holidays for each country/region defined in countries_meta.txt.

Usage:
    python3  update_countries_meta_holidays.py            # Uses the current year
    python3  update_countries_meta_holidays.py --year 2025  # Uses the specified year

This script reads country/region names from countries_meta.txt, and generates separate JSON files containing holidays for each entry.
For regions that share holidays with their base country, symlinks are created instead of duplicate files.
Output files are stored in data/countries/public_holidays/{map_name}.json
"""
import json
import pycountry
import holidays
import os
import argparse
import datetime
import re

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
}

# Setup paths
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, '../..'))
input_file = os.path.join(project_root, 'data', 'countries_meta.txt')
output_dir = os.path.join(project_root, 'data', 'countries', 'public_holidays')

# Create output directory if it doesn't exist
os.makedirs(output_dir, exist_ok=True)

# Parse command line arguments
parser = argparse.ArgumentParser(description="Generate per-country/region public holidays.")
parser.add_argument('--year', type=int, default=datetime.datetime.now().year, help='Year for holidays (default: current year)')
args = parser.parse_args()
year = args.year

# Read countries metadata
with open(input_file, "r", encoding="utf-8") as f:
    meta = json.load(f)

UNWANTED_PHRASES = [
    "observed",
    "in lieu",
    "by old style",
    "by new style",
    "non-working",
    "substituted from",
    "National",
    "substitute day"
]

def clean_holiday_name(name):
    for phrase in UNWANTED_PHRASES:
        name = name.replace(f" ({phrase})", "")
        name = name.replace(f" ({phrase.title()})", "")  # handles capitalized versions

    # Special case: remove parentheses starting with "substituted from"
    name = re.sub(r"\s*\(substituted from [^)]+\)", "", name, flags=re.IGNORECASE) 
    return name.strip()

# First, process base countries (without underscore)
base_countries = {}
for map_name in meta.keys():
    if '_' not in map_name:  # Process only base countries first
        try:
            country = pycountry.countries.lookup(map_name)
            iso_code = country.alpha_2
        except LookupError:
            iso_code = custom_name_to_iso.get(map_name)
            if not iso_code:
                print(f"[!] Could not find ISO code for: {map_name}")
                continue

        try:
            country_holidays = holidays.country_holidays(iso_code, years=range(year, year + 2))
            holidays_dict = {
                str(date): clean_holiday_name(holiday_name)
                for date, holiday_name in country_holidays.items()
            }
            # Save base country holidays
            output_file = os.path.join(output_dir, f"{map_name}.json")
            with open(output_file, "w", encoding="utf-8") as f:
                json.dump(holidays_dict, f, indent=2, ensure_ascii=False)
            base_countries[map_name] = output_file
        except Exception as e:
            print(f"[!] Holidays not available for {map_name} ({iso_code}): {e}")
            base_countries[map_name] = None

# Then process regions, creating symlinks to their base country's file
for map_name in meta.keys():
    if '_' in map_name:  # Process only regions
        base_name = map_name.split('_')[0]
        if base_name in base_countries and base_countries[base_name]:
            # Create symlink to base country's holiday file
            region_file = os.path.join(output_dir, f"{map_name}.json")
            base_file = f"{base_name}.json"
            
            # Remove existing file/symlink if it exists
            if os.path.exists(region_file):
                os.remove(region_file)
            
            # Create relative symlink
            os.symlink(base_file, region_file)
        else:
            print(f"[!] Base country {base_name} not available for region {map_name}")

