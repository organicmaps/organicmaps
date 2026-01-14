#!/usr/bin/env python3

import json
from typing import Set

from common import get_countries_meta_timezones, TIMEZONE_INFO_FILE_PATH


def read_timezones_from_tz_info() -> Set[str]:
    with open(TIMEZONE_INFO_FILE_PATH, 'r') as f:
        return set(json.load(f)['timezones'].keys())


def validate_timezones_match_metadata() -> bool:
    meta_timezones = get_countries_meta_timezones()
    timezones = read_timezones_from_tz_info()

    only_in_meta = meta_timezones - timezones
    only_in_info = timezones - meta_timezones

    is_ok = True

    if only_in_meta:
        print(f"Missing timezones: {only_in_meta}")
        is_ok = False

    if only_in_info:
        print(f"Unused timezones: {only_in_info}")
        is_ok = False

    if not is_ok:
        print("Please regenerate timezone_info.json.")

    return is_ok


if __name__ == "__main__":
    result = validate_timezones_match_metadata()
    if not result:
        exit(1)
