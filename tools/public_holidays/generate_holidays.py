"""
Script to generate and save public holidays for all available countries for a specific year
using the `holidays` Python library.

Usage:
    python3 generate_holidays.py            # Uses the current year
    python3 generate_holidays.py --year 2025  # Uses the specified year

Outputs:
- JSON file with all countries' holidays: data/holidays/public_holidays_all.json
- Separate JSON files per country: data/holidays/countries/{ISO}.json
"""
import holidays
import json
import os
import argparse
import datetime

parser = argparse.ArgumentParser(description="Generate public holidays for all countries.")
parser.add_argument('--year', type=int, default=datetime.datetime.now().year, help='Year for holidays (default: current year)')
args = parser.parse_args()

year = args.year

script_dir = os.path.dirname(os.path.abspath(__file__))
project_root = os.path.abspath(os.path.join(script_dir, '../..'))
all_output_dir = os.path.join(project_root, 'data', 'holidays')
per_country_output_dir = os.path.join(all_output_dir, 'countries')
os.makedirs(all_output_dir, exist_ok=True)
os.makedirs(per_country_output_dir, exist_ok=True)

all_data = {}

for country_code in holidays.list_supported_countries():
    try:
        country_holidays = holidays.country_holidays(country_code,  years=range(year, year + 2))
        holiday_list =  {
            
              str(date): holiday_name
              for date, holiday_name in country_holidays.items()
        }
        # Save per-country file
        with open(f"{per_country_output_dir}/{country_code}.json", "w", encoding="utf-8") as f:
            json.dump(holiday_list, f, indent=2, ensure_ascii=False)
        # Add to all_data
        all_data[country_code] = holiday_list
    except Exception as e:
        print(f"Failed for {country_code}: {e}")

# Save all countries in one file
all_output_file = os.path.join(all_output_dir, 'public_holidays_all.json')
with open(all_output_file, "w", encoding="utf-8") as f:
    json.dump(all_data, f, indent=2, ensure_ascii=False)


