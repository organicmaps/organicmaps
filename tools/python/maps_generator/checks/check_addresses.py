import re

from maps_generator.checks import check
from maps_generator.checks.logs import logs_reader


ADDR_PATTERN = re.compile(
    r".*BuildAddressTable\(\) Address: "
    r"Matched percent (?P<matched_percent>[0-9.]+) "
    r"Total: (?P<total>\d+) "
    r"Missing: (?P<missing>\d+)"
)


class AddrInfo:
    def __init__(self, matched_percent, total, missing):
        self.matched_percent = float(matched_percent)
        self.total = float(total)
        self.matched = self.total - float(missing)

    def __str__(self):
        return f"Matched percent: {self.matched_percent}, total: {self.total}, matched: {self.matched}"


def get_addresses_check_set(old_path: str, new_path: str) -> check.CompareCheckSet:
    def do(path: str):
        log = logs_reader.Log(path)
        if not log.is_country_log:
            return None

        found = logs_reader.find_and_parse(log.lines, ADDR_PATTERN)
        if not found:
            return None

        d = found[0][0]
        return AddrInfo(**d)

    def op(lhs: AddrInfo, rhs: AddrInfo):
        return lhs.matched_percent - rhs.matched_percent

    return check.build_check_set_for_files(
        "Addresses check", old_path, new_path, do=do, op=op
    )
