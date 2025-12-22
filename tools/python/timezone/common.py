import json
import zoneinfo
from pathlib import Path
from typing import Set, Optional

OM_ROOT = Path(__file__).parent.parent.parent.parent
COUNTRIES_META_FILE_PATH = OM_ROOT / "data" / "countries_meta.txt"
TIMEZONE_INFO_FILE_PATH = OM_ROOT / "data" / "generator" / "timezone" / "timezone_info.json"


def get_countries_meta_timezones(data: dict = None) -> Set[str]:
    if data is None:
        data = json.loads(open(COUNTRIES_META_FILE_PATH, 'r').read())
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
    tzdb_path = Path(zoneinfo.TZPATH[0])
    version_file = tzdb_path / "+VERSION"
    if version_file.exists():
        return version_file.read_text().strip()
    return None
