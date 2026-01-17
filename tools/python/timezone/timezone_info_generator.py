#!/usr/bin/env python3

import argparse
import json
from datetime import datetime, timedelta, timezone
from typing import Set
from zoneinfo import ZoneInfo

from common import TIMEZONE_INFO_FILE_PATH, get_countries_meta_timezones, get_local_tzdb_version

EPOCH = datetime(1970, 1, 1, tzinfo=timezone.utc)
INITIAL_YEAR = 2026


def day_index(dt):
    """Return number of days since epoch."""
    return (dt - EPOCH).days


def generate_timezone_data(tz_name: str, start_year: int, end_year: int) -> dict[str, int]:
    tz = ZoneInfo(tz_name)
    transitions = []
    last_offset = None

    dt = datetime(start_year, 1, 1, tzinfo=timezone.utc)
    end = datetime(end_year + 1, 1, 1, tzinfo=timezone.utc)

    while dt < end:
        local = dt.astimezone(tz)
        offset_min = int(local.utcoffset().total_seconds() // 60)

        if last_offset is not None and offset_min != last_offset:
            transitions.append({
                "utc_day": day_index(dt),
                "minute_of_day": dt.hour * 60 + dt.minute,
                "offset": offset_min
            })

        last_offset = offset_min
        dt += timedelta(hours=1)

    if transitions:
        offsets = [t["offset"] for t in transitions]
        base_offset = min(offsets)
        dst_delta = max(offsets) - base_offset
    else:
        base_offset = int(
            datetime(start_year, 1, 1, tzinfo=timezone.utc).astimezone(tz).utcoffset().total_seconds() // 60)
        dst_delta = 0

    # Convert transitions to day_delta and is_dst
    prev_day = day_index(datetime(start_year, 1, 1, tzinfo=timezone.utc))
    final_transitions = []
    for t in transitions:
        day_delta = t["utc_day"] - prev_day
        prev_day = t["utc_day"]
        final_transitions.append({
            "day_delta": day_delta,
            "minute_of_day": t["minute_of_day"],
        })

    return {
        "base_offset": int((base_offset / 15) + 64),
        "dst_delta": dst_delta,
        "transitions": final_transitions
    }


def generate_timezones_data(timezones: Set[str], start_year: int, years: int) -> dict:
    result = {}
    for tz_name in sorted(timezones):
        print(f"Generating timezone data for {tz_name}")
        result[tz_name] = generate_timezone_data(tz_name, start_year, start_year + years - 1)
    return result


if __name__ == "__main__":
    parser = argparse.ArgumentParser(prog="timezone_info_generator")
    parser.add_argument("--output", type=str, default=TIMEZONE_INFO_FILE_PATH)
    parser.add_argument("--start-year", type=int, required=True, help="Year to start generation from.")
    parser.add_argument("--years", type=int, required=False, default=2, help="Number of years to generate.")

    args = parser.parse_args()

    timezone_info = {
        "tzdb_version": get_local_tzdb_version(),
        "tzdb_format_version": 0,
        "tzdb_generation_year_offset": args.start_year - INITIAL_YEAR,
        "timezones": generate_timezones_data(get_countries_meta_timezones(), args.start_year, args.years)
    }

    with open(TIMEZONE_INFO_FILE_PATH, "w") as f:
        f.write(json.dumps(timezone_info, indent=2))
