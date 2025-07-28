"""
Script to generate public holidays for each country/region defined in countries_meta.txt.

Usage:
    python3  update_countries_meta_holidays.py            # Uses the current year
    python3  update_countries_meta_holidays.py --year 2025  # Uses the specified year

This script reads country/region names from countries_meta.txt, and generates separate JSON files containing holidays for each entry.
Output files are stored in data/public_holidays/{map_name}.json
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
}

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


# Setup paths
script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, '../..'))
input_file = os.path.join(project_root, 'data', 'countries', 'metadata.json')
output_dir = os.path.join(project_root, 'data', 'countries', 'public_holidays')

# Create output directory if it doesn't exist
os.makedirs(output_dir, exist_ok=True)

# Parse command line arguments
parser = argparse.ArgumentParser(description="Generate per-country/region public holidays.")
parser.add_argument('--year', type=int, default=datetime.datetime.now().year, help='Year for holidays (default: current year)')
parser.add_argument('--country', type=str, default=None, 
    help='Update only this country/region (e.g., "Spain", "Spain_Catalonia"). Names must match countries_meta.txt')
args = parser.parse_args()
year = args.year
country = args.country

# Read countries metadata
with open(input_file, "r", encoding="utf-8") as f:
    meta = json.load(f)

def clean_holiday_name(name):
    for phrase in UNWANTED_PHRASES:
        name = name.replace(f" ({phrase})", "")
        name = name.replace(f" ({phrase.title()})", "")  # handles capitalized versions

    # Special case: remove parentheses starting with "substituted from"
    name = re.sub(r"\s*\(substituted from [^)]+\)", "", name, flags=re.IGNORECASE) 
    return name.strip()

if args.country and args.country not in meta:
    print(f"Error: {args.country} not found in countries_meta.txt")
    sys.exit(1)

# Process each country/region from the metadata
for map_name in meta.keys():

    if args.country and map_name != country:
        continue

    # Get base country name (before underscore if it's a region)
    base_name = map_name.split("_")[0]
    
    # Step 1: Get ISO code for the base country
    try:
        country = pycountry.countries.lookup(base_name)
        iso_code = country.alpha_2
    except LookupError:
        iso_code = custom_name_to_iso.get(base_name)
        if not iso_code:
            print(f"[!] Could not find ISO code for: {base_name}")
            continue

    # Step 2: Add public holidays if available
    try:
        country_holidays = holidays.country_holidays(iso_code, years=range(year, year + 2))
        holidays_dict = {
            str(date): clean_holiday_name(holiday_name)
            for date, holiday_name in country_holidays.items()
        }
    except Exception as e:
        print(f"[!] Holidays not available for {map_name} ({iso_code}): {e}")
        holidays_dict = {}  # Empty dict for countries/regions without holiday data

    # Step 3: Write holidays to a separate JSON file
    output_file = os.path.join(output_dir, f"{map_name}.json")
    with open(output_file, "w", encoding="utf-8") as f:
        json.dump(holidays_dict, f, indent=2, ensure_ascii=False)

