"""
Script to generate and save public holidays for all available countries for a specific year
using the `holidays` Python library.

Output: JSON file with country codes and holiday dates.
"""
import holidays
import json
import os

script_dir = os.path.dirname(os.path.abspath(__file__))
output_dir = os.path.join(script_dir, '../../../data/holidays')
os.makedirs(output_dir, exist_ok=True)
output_file = os.path.join(output_dir, 'public_holidays_all.json')

year = 2025

all_data = {}

for country_code in holidays.list_supported_countries():
    try:
        country_holidays = holidays.country_holidays(country_code, years=year)
        all_data[country_code] = [
            {
                "date": str(date),
                "name": name
            } for date, name in country_holidays.items()
        ]
    except Exception as e:
        print(f"Failed for {country_code}: {e}")

with open(output_file, "w", encoding="utf-8") as f:
    json.dump(all_data, f, indent=2, ensure_ascii=False)


