import re
from datetime import timedelta


DURATION_PATTERN = re.compile(
    r"((?P<days>[-\d]+) day[s]*, )?(?P<hours>\d+):(?P<minutes>\d+):(?P<seconds>\d[\.\d+]*)"
)


def unique(s):
    seen = set()
    seen_add = seen.add
    return [x for x in s if not (x in seen or seen_add(x))]


def parse_timedelta(s):
    m = DURATION_PATTERN.match(s)
    d = m.groupdict()
    return timedelta(**{k: float(d[k]) for k in d if d[k] is not None})
