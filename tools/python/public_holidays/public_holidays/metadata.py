import json
from pathlib import Path

# Mix of ISO 3166-1 alpha-2 (e.g. "DE") and ISO 3166-2 (e.g. "BE-VAN") codes
IsoCodes = list[str]


def parse_iso_codes(path: str | Path) -> IsoCodes:
    with open(path, encoding="utf-8") as f:
        data: dict = json.load(f)

    values: IsoCodes = []

    for entry in data.values():
        if "iso3166-1" in entry:
            alpha2 = entry["iso3166-1"].get("alpha-2")
            if alpha2:
                values.append(alpha2)
        if "iso3166-2" in entry:
            values.extend(entry["iso3166-2"])

    return sorted(values)
