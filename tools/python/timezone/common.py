import json
import zoneinfo
from pathlib import Path
from typing import Set, Optional

OM_ROOT = Path(__file__).parent.parent.parent.parent
COUNTRIES_META_FILE_PATH = OM_ROOT / "data" / "countries_meta.txt"
TIMEZONE_INFO_FILE_PATH = OM_ROOT / "data" / "generator" / "timezone" / "timezone_info.json"


def get_countries_meta_timezones(data: dict = None) -> Set[str]:
    if data is None:
        data = json.loads(COUNTRIES_META_FILE_PATH.read_text(encoding="utf-8"))
    timezones = set()
    if isinstance(data, dict):
        for key, value in data.items():
            if key == "timezone":
                timezones.add(value)
            timezones.update(get_countries_meta_timezones(value))
    elif isinstance(data, list):
        for item in data:
            timezones.update(get_countries_meta_timezones(item))
    return timezones


def get_local_tzdb_version() -> Optional[str]:
    for tzdb_path in zoneinfo.TZPATH:
        version_file = Path(tzdb_path) / "+VERSION"  # Mac OS
        if version_file.exists():
            return version_file.read_text().strip()
        version_file = Path(tzdb_path) / "tzdata.zi"  # Ubuntu
        if version_file.exists():
            with open(version_file, "r") as f:
                header = f.readline().strip();
                if header.startswith("# version "):
                    return header[10:]
    return None
