"""
Script to generate and save public holidays for a single country using its ISO code and the `holidays` Python library.

Output: JSON files for each country with holiday dates.
"""
import holidays
import json
import os

year = 2025

script_dir = os.path.dirname(os.path.abspath(__file__))
output_dir = os.path.join(script_dir, '../../../data/holidays/countries')
os.makedirs(output_dir, exist_ok=True)

for country_code in holidays.list_supported_countries():
    try:
        country_holidays = holidays.country_holidays(country_code, years=year)
        holiday_list = [
            {
                "date": str(date),
                "name": name
            } for date, name in country_holidays.items()
        ]
        with open(f"{output_dir}/{country_code}.json", "w", encoding="utf-8") as f:
            json.dump(holiday_list, f, indent=2, ensure_ascii=False)
    except Exception as e:
        print(f"Failed for {country_code}: {e}")
