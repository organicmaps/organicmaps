"""
Script to enrich a countries metadata file (e.g., `countries_meta.txt`) with ISO codes and public holidays.

Usage:
    python3  update_countries_meta_holidays.py            # Uses the current year
    python3  update_countries_meta_holidays.py --year 2025  # Uses the specified year

This script attempts to match country names, fetch holiday data where available, and
output the result as a structured JSON file.
"""
import json
import pycountry
import holidays
import os
import argparse
import datetime
import re

script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, '../..'))
input_file = os.path.join(project_root, 'data', 'countries_meta.txt')
output_file = os.path.join(project_root, 'data', 'countries_meta.json')

parser = argparse.ArgumentParser(description="Update countries_meta.json with ISO codes and public holidays.")
parser.add_argument('--year', type=int, default=datetime.datetime.now().year, help='Year for holidays (default: current year)')
args = parser.parse_args()

year = args.year

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

# Clean function
def clean_holiday_name(name):
    for phrase in UNWANTED_PHRASES:
        name = name.replace(f" ({phrase})", "")
        name = name.replace(f" ({phrase.title()})", "")  # handles capitalized versions

    # Special case: remove parentheses starting with "substituted from"
    name = re.sub(r"\s*\(substituted from [^)]+\)", "", name, flags=re.IGNORECASE) 
    return name.strip()

for name, data in meta.items():
    base_name = name.split("_")[0]  # Use only country part, e.g., "Spain" from "Spain_Catalonia"
    # Step 1: Add ISO code
    try:
        country = pycountry.countries.lookup(base_name)
        iso_code = country.alpha_2
        data["_iso_code"] = iso_code
    except LookupError:
        print(f"[!] Could not find ISO code for: {base_name}")
        continue

    # Step 2: Add public holidays if available
    try:
        country_holidays = holidays.country_holidays(iso_code, years=range(year, year + 2))
        data["public_holidays"] = {
              str(date): clean_holiday_name(holiday_name)
              for date, holiday_name in country_holidays.items()
        }
    except Exception as e:
        print(f"[!] Holidays not available for {base_name} ({iso_code}): {e}")
        continue

# Remove iso_code from output
for data in meta.values():
    data.pop("_iso_code", None)

# Save the enriched JSON
with open(output_file, "w", encoding="utf-8") as f:
    json.dump(meta, f, indent=2, ensure_ascii=False)

