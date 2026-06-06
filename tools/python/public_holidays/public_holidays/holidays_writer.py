import json
import shutil

from .config import Output
from .holiday import HolidayDb
from .metadata import IsoCodes


def write_holidays_per_iso_code(holiday_db: HolidayDb, iso_codes: IsoCodes, output: Output) -> None:
    shutil.rmtree(output.data, ignore_errors=True)
    for iso_code in iso_codes:
        holidays = holiday_db.get_holidays_for_country(iso_code)
        out_dict = {}  # date string (ISO 8601): holiday_name
        for holiday in holidays:
            for date in holiday.holiday_date[iso_code]:
                date_str = date.isoformat()
                if date_str not in out_dict:
                    out_dict[date_str] = []
                out_dict[date_str].append(holiday.name.camel_case_name)
        out_dict = {k: sorted(v) for k, v in sorted(out_dict.items())}
        if not out_dict:
            continue
        output_file = output.data / f"{iso_code}.json"
        output_file.parent.mkdir(parents=True, exist_ok=True)
        output_file.write_text(json.dumps(out_dict, ensure_ascii=False, indent=2) + "\n")
