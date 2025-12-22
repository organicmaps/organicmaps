#!/usr/bin/env python3

import json
from datetime import datetime, timedelta, timezone
from typing import Set
from zoneinfo import ZoneInfo

from common import TIMEZONE_INFO_FILE_PATH, get_countries_meta_timezones, get_local_tzdb_version

EPOCH = datetime(1970, 1, 1, tzinfo=timezone.utc)


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
        is_dst = 1 if t["offset"] > base_offset else 0
        final_transitions.append({
            "day_delta": day_delta,
            "minute_of_day": t["minute_of_day"],
            "is_dst": is_dst
        })

    return {
        "base_offset": int((base_offset / 15) + 64),
        "dst_delta": dst_delta,
        "transitions": final_transitions
    }


def generate_timezones_data(timezones: Set[str]) -> dict:
    result = {}
    for tz_name in sorted(timezones):
        print(f"Generating timezone data for {tz_name}")
        result[tz_name] = generate_timezone_data(tz_name, 2025, 2026)
    return result


timezone_info = {
    "tzdb_version": get_local_tzdb_version(),
    "tzdb_generation_year_offset": 2025 - 2025,
    "timezones": generate_timezones_data(get_countries_meta_timezones())
}

open(TIMEZONE_INFO_FILE_PATH, "w").write(json.dumps(timezone_info, indent=2))
