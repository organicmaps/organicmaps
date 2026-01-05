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
  "tzdb_format_version": 0,
  "tzdb_generation_year_offset": 0,
  "timezones": {
    "Europe/Berlin": {
      "base_offset": 68,
      "dst_delta": 60,
      "transitions": [
        {
          "day_delta": 88,
          "minute_of_day": 120
        },
        {
          "day_delta": 180,
          "minute_of_day": 180
        }
      ]
    },
    "Europe/Moscow": {
      "base_offset": 76,
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

`tzdb_format_version`

* Type: `int` in range `[0, 7]`
* Description: Indicates the version of the binary format used to serialize this JSON.
* Usage: Useful for future updates to ensure compatibility with older versions.

`tzdb_generation_year_offset`

* Type: `int` in range `[0, 63]`
* Description: The generation year of tzdb since 2026 (0-based).
* Usage: Used to interpret transition dates relative to the generation year.
* Example: `0` for 2026, `1` for 2027, etc.

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
* Note: Encoded as Excess-64

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
2. Future-only transitions (2–4 years)
3. Delta encoding to minimize storage

---

## Logical Binary Structure

For each timezone:

| Field                  | Bits | 
|------------------------|------|
| format_version         | 3    |
| generation_year_offset | 6    |
| base_offset            | 7    |
| dst_delta              | 8    |
| transition_count       | 4    |

**Total:** 28 bits → 4 bytes

Each transition encodes a DST change relative to the previous one.

| Field         | Bits |
|---------------|------|
| day_delta     | 9    |
| minute_of_day | 11   |

**Total:** 20 bits → 3 bytes

---

### `base_offset` Excess-64 encoding

The offset is measured in 15-minute steps, shifted by +64 to avoid negative numbers when encoded.
Instead of storing the real value directly, the stored value is:

```
Raw = RealValue + 64
```

To decode it:

```
RealValue = Raw − 64
```

#### Examples:

* 1 hour = 4 units
* 30 minutes = 2 units
* 15 minutes = 1 unit


* UTC+2:00 → 2 hours → +8
* UTC−5:30 → -(5 hours 30 minutes) → −22

Encoding:

```
UTC+2:00 -> (120min / 15) + 64 = 72
UTC−5:30 -> (-330min / 15) + 64 = 42
```

Decoding:

```
72 -> (72 - 64) * 15 = 120min -> UTC+2:00
42 -> (-42 - 64) * 15 = -330min -> UTC-5:30
```

---

### Why delta encoding?

- DST transitions are far apart (months)
- `day_delta` fits easily in 9 bits
- Removes the need for full-timestamps (64-bit)

---

## Example Binary Size

Typical DST-observing timezone (Europe/Berlin, 4 years):

| Component               | Size         |
|-------------------------|--------------|
| Header                  | 28 bits      |
| 8 transitions * 20 bits | 160 bits     |
| **Total**               | **24 bytes** |

Timezone without DST (Europe/Moscow):

| Component   | Size        |
|-------------|-------------|
| Header      | 27 bits     |
| Transitions | 0           |
| **Total**   | **4 bytes** |

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
