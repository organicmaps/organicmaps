# Offline Timezone Database

This JSON file provides a compact, offline representation of selected IANA timezones suitable for offline timezone
conversions in C++ applications.
It contains only the minimal data required for converting between UTC and local times, including DST transitions for
future years.

The JSON file is generated from the [IANA Time Zone Database](https://www.iana.org/time-zones) using
the [timezone_info_generator](../../../tools/python/timezone/timezone_info_generator.py) tool.

The JSON file is generated automatically and should not be modified manually.

The JSON file is generated only for timezones listed in our [countries_meta.txt](../../countries_meta.txt) file.

---

## JSON Structure

```json
{
  "tzdb_version": "2025b",
  "timezones": {
    "Europe/Berlin": {
      "base_offset": 60,
      "dst_delta": 60,
      "transitions": [
        {
          "day_delta": 88,
          "minute_of_day": 120,
          "is_dst": 1
        },
        {
          "day_delta": 180,
          "minute_of_day": 180,
          "is_dst": 0
        }
      ]
    },
    "Europe/Moscow": {
      "base_offset": 180,
      "dst_delta": 0,
      "transitions": []
    }
  }
}
```

### Top-Level Fields

`tzdb_version`

* Type: `string`
* Description: Indicates the version of the underlying tz database used to generate this JSON.
* Usage: Useful for verification and future updates to ensure consistency with the tz rules.
* Example: `2025b`.

`timezones`

* Type: `object`
* Description: A dictionary mapping IANA timezone names to their corresponding offline timezone data.
* Usage: Provides all required information for each timezone to perform offline time conversion.

### Timezone Object Fields

#### Each timezone object contains:

`base_offset`

* Type: `int` (minutes)
* Description: Standard UTC offset during non-DST (winter) time.
* Usage: Added to UTC to get local time during standard time.

`dst_delta`

* Type: `int` (minutes)
* Description: Additional offset applied during daylight saving time (DST).
* Usage: Added to base_offset during DST periods to compute local time.

`transitions`

* Type: `array` of objects
* Description: Chronologically sorted list of DST transitions for the given timezone.
* Note: Only transitions in the future (within the selected year range) are included.

#### Each Transition object contains:

`day_delta`

* Type: `int`
* Description: Number of days since the previous transition (the first transition is relative to the start of the
  selected year).
* Usage: Allows delta-encoded reconstruction of transition dates efficiently for memory-compact storage.

`minute_of_day`

* Type: integer
* Description: Minute of the day when the transition occurs (0–1439).
* Usage: Combined with day_delta to calculate the exact UTC timestamp of the DST change.

`is_dst`

* Type: integer (0 or 1)
* Description: Indicates whether DST is active after this transition.
* Usage: Determines whether dst_delta should be added to base_offset after this transition.

## Notes

The JSON only includes future transitions; historical changes are ignored.
Timezone offsets are stored in minutes, not seconds, for compactness.

## Binary Format and Memory Layout

This JSON file is intended as an **intermediate, human-readable format**.  
At maps generation time, it is converted into a **compact binary representation** optimized for:

- Minimal memory usage
- Fast UTC <-> local conversions
- Offline operation
- Predictable size

The binary format mirrors the JSON data but removes redundancy, text overhead, and unused precision.

---

## Design Goals of the Binary Format

1. Minute-level precision only
3. Future-only transitions (2–4 years)
4. Delta encoding to minimize storage

---

## Logical Binary Structure

For each timezone:

| Field            | Bits | Description                                 |
|------------------|------|---------------------------------------------|
| base_offset      | 12   | Standard UTC offset in minutes (−780..+780) |
| dst_delta        | 8    | DST adjustment in minutes (usually 60)      |
| transition_count | 4    | Number of transitions (0–15)                |

**Total:** 24 bits

Each transition encodes a DST change relative to the previous one.

| Field         | Bits | Description                              |
|---------------|------|------------------------------------------|
| day_delta     | 9    | Days since previous transition (0–511)   |
| minute_of_day | 11   | Minute of day (0–1439)                   |
| is_dst        | 1    | Whether DST is active *after* transition |

**Total:** 21 bits

### Why delta encoding?

- DST transitions are far apart (months)
- `day_delta` fits easily in 9 bits
- Removes the need for full-timestamps (64-bit)

---

## Example Binary Size

Typical DST-observing timezone (Europe/Berlin, 4 years):

| Component               | Size         |
|-------------------------|--------------|
| Header                  | 3 bytes      |
| 8 transitions * 21 bits | 21 bytes     |
| **Total**               | **24 bytes** |

Timezone without DST (Europe/Moscow):

| Component   | Size        |
|-------------|-------------|
| Header      | 3 bytes     |
| Transitions | 0           |
| **Total**   | **3 bytes** |

---

## Comparison with Other Formats

| Format               | Size per TZ    |
|----------------------|----------------|
| Full tzdb (zoneinfo) | 20–50 KB       |
| ICU timezone data    | ~5–10 KB       |
| This binary format   | **3–25 bytes** |

---

## Conversion Flow

* [IANA Time Zone Database](https://www.iana.org/time-zones)
* [timezone_info_generator.py](../../../tools/python/timezone/timezone_info_generator.py)
* [timezone_info.json](timezone_info.json)
* JSON to Binary serialization during maps generation
* Binary deserialization at runtime
* UTC <-> local TZ time conversion

---

## Why Offsets Are Stored in Minutes (Not Seconds)

- All modern tz rules use minute precision
- Historical sub-minute offsets are irrelevant for future-only support
- Saves bits and simplifies arithmetic
- Matches POSIX and most OS-level representations

---

## Ambiguous and Skipped Times

- **UTC -> local:** always unambiguous
- **Local -> UTC:** resolved by:
    - Iterative offset correction
    - Consistent DST rule application
- Leap seconds are intentionally ignored (same as POSIX time)

---
