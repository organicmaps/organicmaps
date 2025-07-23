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

for name, data in meta.items():
    # Step 1: Add ISO code
    try:
        country = pycountry.countries.lookup(name)
        iso_code = country.alpha_2
        data["iso_code"] = iso_code
    except LookupError:
        print(f"[!] Could not find ISO code for: {name}")
        continue

    # Step 2: Add public holidays if available
    try:
        country_holidays = holidays.country_holidays(iso_code, years=year)
        data["public_holidays"] = [
            {
                "date": str(date),
                "name": holiday_name
            } for date, holiday_name in country_holidays.items()
        ]
    except Exception as e:
        print(f"[!] Holidays not available for {name} ({iso_code}): {e}")
        continue

# Save the enriched JSON
with open(output_file, "w", encoding="utf-8") as f:
    json.dump(meta, f, indent=2, ensure_ascii=False)

