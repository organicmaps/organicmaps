import re

from maps_generator.checks import check
from maps_generator.checks.logs import logs_reader


ADDR_PATTERN = re.compile(
    r".*BuildAddressTable\(\) Address: "
    r"Matched percent (?P<matched_percent>[0-9.]+) "
    r"Total: (?P<total>\d+) "
    r"Missing: (?P<missing>\d+)"
)


def get_addresses_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    """
    Returns an addresses check set, that checks a difference in 'matched_percent'
    addresses of BuildAddressTable between old logs and new logs.
    """
    def do(path: str):
        log = logs_reader.Log(path)
        if not log.is_mwm_log:
            return None

        found = logs_reader.find_and_parse(log.lines, ADDR_PATTERN)
        if not found:
            return None

        d = found[0][0]
        return float(d["matched_percent"])

    return check.build_check_set_for_files(
        "Addresses check", old_path, new_path, ext=".log", do=do
    )
