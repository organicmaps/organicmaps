import datetime
import json
import logging
import os
import re
from collections import defaultdict
from typing import AnyStr
from typing import Dict
from typing import List

from maps_generator.generator.env import WORLDS_NAMES
from maps_generator.generator.exceptions import ParseError

logger = logging.getLogger("maps_generator")

RE_STAT = re.compile(
    r"(?:\d+\. )?([\w:|-]+?)\|: "
    r"size = \d+; "
    r"count = (\d+); "
    r"length = ([0-9.e+-]+) m; "
    r"area = ([0-9.e+-]+) m²; "
    r"names = (\d+)\s*"
)

RE_TIME_DELTA = re.compile(
    r"^(?:(?P<days>-?\d+) (days?, )?)?"
    r"((?:(?P<hours>-?\d+):)(?=\d+:\d+))?"
    r"(?:(?P<minutes>-?\d+):)?"
    r"(?P<seconds>-?\d+)"
    r"(?:\.(?P<microseconds>\d{1,6})\d{0,6})?$"
)

RE_FINISH_STAGE = re.compile(r"(.*)Stage (.+): finished in (.+)$")


def read_stat(f):
    stats = []
    for line in f:
        m = RE_STAT.match(line)
        stats.append(
            {
                "name": m.group(1).replace("|", "-"),
                "cnt": int(m.group(2)),
                "len": float(m.group(3)),
                "area": float(m.group(4)),
                "names": int(m.group(5)),
            }
        )
    return stats


def read_config(f):
    config = []
    for line in f:
        l = line.strip()
        if l.startswith("#") or not l:
            continue
        columns = [c.strip() for c in l.split(";", 2)]
        columns[0] = re.compile(columns[0])
        columns[1] = columns[1].lower()
        config.append(columns)
    return config


def process_stat(config, stats):
    result = {}
    for param in config:
        res = 0
        for t in stats:
            if param[0].match(t["name"]):
                if param[1] == "len":
                    res += t["len"]
                elif param[1] == "area":
                    res += t["area"]
                elif param[1] == "cnt_names":
                    res += t["names"]
                else:
                    res += t["cnt"]
        result[str(param[0]) + param[1]] = res
    return result


def format_res(res, t):
    unit = None
    if t == "len":
        unit = "m"
    elif t == "area":
        unit = "m²"
    elif t == "cnt" or t == "cnt_names":
        unit = "pc"
    else:
        raise ParseError(f"Unknown type {t}.")

    return res, unit


def make_stats(config_path, stats_path):
    with open(config_path) as f:
        config = read_config(f)
    with open(stats_path) as f:
        stats = process_stat(config, read_stat(f))
    lines = []
    for param in config:
        k = str(param[0]) + param[1]
        st = format_res(stats[k], param[1])
        lines.append({"type": param[2], "quantity": st[0], "unit": st[1]})
    return lines


def parse_time(time_str):
    parts = RE_TIME_DELTA.match(time_str)
    if not parts:
        return
    parts = parts.groupdict()
    time_params = {}
    for name, param in parts.items():
        if param:
            time_params[name] = int(param)
    return datetime.timedelta(**time_params)


def get_stages_info(log_path, ignored_stages=frozenset()):
    result = defaultdict(lambda: defaultdict(dict))
    for file in os.listdir(log_path):
        path = os.path.join(log_path, file)
        with open(path) as f:
            for line in f:
                m = RE_FINISH_STAGE.match(line)
                if not m:
                    continue
                stage_name = m.group(2)
                dt = parse_time(m.group(3))
                if file.startswith("stage_") and stage_name not in ignored_stages:
                    result["stages"][stage_name] = dt
                else:
                    country = file.split(".")[0]
                    result["countries"][country][stage_name] = dt
    return result


def read_types(path: AnyStr) -> Dict[AnyStr, Dict]:
    """"
    Reads and summarizes statistics for all countries, excluding World and
    WorldCoast.
    """
    with open(path) as f:
        json_data = json.load(f)
        all_types = {}
        countries = json_data["countries"]
        for country, json_value in countries.items():
            if country in WORLDS_NAMES:
                continue
            try:
                json_types = json_value["types"]
            except KeyError:
                logger.exception(f"Cannot parse {json_value}")
                continue
            for t in json_types:
                curr = all_types.get(t["type"], {})
                curr["quantity"] = curr.get("quantity", 0.0) + t["quantity"]
                curr["unit"] = t["unit"]
                all_types[t["type"]] = curr
        return all_types


def diff(new: Dict[AnyStr, Dict], old: Dict[AnyStr, Dict]) -> List:
    assert len(new) == len(old)
    lines = []
    for key in new:
        o = old[key]["quantity"]
        n = new[key]["quantity"]
        rel = 0
        if o != 0.0:
            rel = int(((n - o) / o) * 100)
        else:
            if n != 0.0:
                rel = 100

        lines.append((key, o, n, rel, n - o, new[key]["unit"],))
    return lines
