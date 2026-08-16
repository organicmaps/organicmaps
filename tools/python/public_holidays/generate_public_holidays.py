#!/usr/bin/env python3

from pathlib import Path
from public_holidays.config import parse_config
from public_holidays.metadata import parse_iso_codes
from public_holidays.data_source import create_data_source
from public_holidays.holiday import HolidayDb
from public_holidays.holidays_writer import write_holidays_per_iso_code
from public_holidays.twine import load_from_twine, write_to_twine
from public_holidays.code_generator import generate_code

OM_ROOT = Path(__file__).parent.parent.parent
DEFAULT_CONFIG_PATH = Path(__file__).parent / "config.yaml"

if __name__ == "__main__":
    config = parse_config(DEFAULT_CONFIG_PATH)
    iso_codes = parse_iso_codes(config.input.metadata)

    holiday_db = HolidayDb()

    load_from_twine(holiday_db, config.twine.path)

    for data_source in config.data_sources:
        if not data_source.enabled:
            print(f"[SKIP] {data_source.name} is disabled")
            continue
        ds = create_data_source(data_source.name, data_source.supported_countries, config.duplicates_map,
                                config.twine.supported_languages,
                                holiday_db,
                                config.input.holiday_types)
        ds.load_holidays(iso_codes, config.input.generation_years)

    write_holidays_per_iso_code(holiday_db, iso_codes, config.output)
    write_to_twine(holiday_db, config.twine.path)
    generate_code(holiday_db, config.output.sources, Path(__file__).relative_to(OM_ROOT))
