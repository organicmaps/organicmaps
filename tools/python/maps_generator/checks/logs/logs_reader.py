import datetime
import logging
import os
import re
from collections import Counter
from collections import namedtuple
from enum import Enum
from pathlib import Path
from typing import List
from typing import Tuple
from typing import Union

import maps_generator.generator.env as env
from maps_generator.generator.stages import get_stage_type
from maps_generator.utils.algo import parse_timedelta


logger = logging.getLogger(__name__)


FLAGS = re.MULTILINE | re.DOTALL

GEN_LINE_PATTERN = re.compile(
    r"^LOG\s+TID\((?P<tid>\d+)\)\s+(?P<level>[A-Z]+)\s+"
    r"(?P<timestamp>[-.e0-9]+)\s+(?P<message>.+)$",
    FLAGS,
)
GEN_LINE_CHECK_PATTERN = re.compile(
    r"^TID\((?P<tid>\d+)\)\s+" r"ASSERT FAILED\s+(?P<message>.+)$", FLAGS
)

MAPS_GEN_LINE_PATTERN = re.compile(
    r"^\[(?P<time_string>[0-9-:, ]+)\]\s+(?P<level>\w+)\s+"
    r"(?P<module>\w+)\s+(?P<message>.+)$",
    FLAGS,
)

STAGE_START_MSG_PATTERN = re.compile(r"^Stage (?P<name>\w+): start ...$")
STAGE_FINISH_MSG_PATTERN = re.compile(
    r"^Stage (?P<name>\w+): finished in (?P<duration_string>.+)$"
)

LogLine = namedtuple("LogLine", ["timestamp", "level", "tid", "message", "type"])
LogStage = namedtuple("LogStage", ["name", "duration", "lines"])


class LogType(Enum):
    gen = 1
    maps_gen = 2


class Log:
    def __init__(self, path: str):
        self.path = Path(path)
        self.name = self.path.stem

        self.is_stage_log = False
        self.is_mwm_log = False
        try:
            get_stage_type(self.name)
            self.is_stage_log = True
        except AttributeError:
            if self.name in env.COUNTRIES_NAMES or self.name in env.WORLDS_NAMES:
                self.is_mwm_log = True

        self.lines = self._parse_lines()

    def _parse_lines(self) -> List[LogLine]:
        logline = ""
        state = None
        lines = []
        base_timestamp = 0.0

        def try_parse_and_insert():
            nonlocal logline
            logline = logline.strip()
            if not logline:
                return

            nonlocal base_timestamp
            line = None
            if state == LogType.gen:
                line = Log._parse_gen_line(logline, base_timestamp)
            elif state == LogType.maps_gen:
                line = Log._parse_maps_gen_line(logline)
                base_timestamp = line.timestamp

            if line is not None:
                lines.append(line)
            else:
                logger.warn(f"{self.name}: line was not parsed: {logline}")
            logline = ""

        with self.path.open() as logfile:
            for line in logfile:
                if line.startswith("LOG") or line.startswith("TID"):
                    try_parse_and_insert()
                    state = LogType.gen
                elif line.startswith("["):
                    try_parse_and_insert()
                    state = LogType.maps_gen
                logline += line
            try_parse_and_insert()

        return lines

    @staticmethod
    def _parse_gen_line(line: str, base_time: float = 0.0) -> LogLine:
        m = GEN_LINE_PATTERN.match(line)
        if m:
            return LogLine(
                timestamp=base_time + float(m["timestamp"]),
                level=logging.getLevelName(m["level"]),
                tid=int(m["tid"]),
                message=m["message"],
                type=LogType.gen,
            )

        m = GEN_LINE_CHECK_PATTERN.match(line)
        if m:
            return LogLine(
                timestamp=None,
                level=logging.getLevelName("CRITICAL"),
                tid=None,
                message=m["message"],
                type=LogType.gen,
            )

        assert False, line

    @staticmethod
    def _parse_maps_gen_line(line: str) -> LogLine:
        m = MAPS_GEN_LINE_PATTERN.match(line)
        time_string = m["time_string"].split(",")[0]
        timestamp = datetime.datetime.strptime(
            time_string, logging.Formatter.default_time_format
        ).timestamp()
        if m:
            return LogLine(
                timestamp=float(timestamp),
                level=logging.getLevelName(m["level"]),
                tid=None,
                message=m["message"],
                type=LogType.maps_gen,
            )

        assert False, line


class LogsReader:
    def __init__(self, path: str):
        self.path = os.path.abspath(os.path.expanduser(path))

    def __iter__(self):
        for filename in os.listdir(self.path):
            if filename.endswith(".log"):
                yield Log(os.path.join(self.path, filename))


def split_into_stages(log: Log) -> List[LogStage]:
    log_stages = []
    name = None
    lines = []
    for line in log.lines:
        if line.message.startswith("Stage"):
            m = STAGE_START_MSG_PATTERN.match(line.message)
            if m:
                if name is not None:
                    logger.warn(f"{log.name}: stage {name} has not finish line.")
                    log_stages.append(LogStage(name=name, duration=None, lines=lines))
                name = m["name"]

            m = STAGE_FINISH_MSG_PATTERN.match(line.message)
            if m:
                # assert name == m["name"], line
                duration = parse_timedelta(m["duration_string"])
                log_stages.append(LogStage(name=name, duration=duration, lines=lines))
                name = None
                lines = []
        else:
            lines.append(line)

    if name is not None:
        logger.warn(f"{log.name}: stage {name} has not finish line.")
        log_stages.append(LogStage(name=name, duration=None, lines=lines))

    return log_stages


def _is_worse(lhs: LogStage, rhs: LogStage) -> bool:
    if (lhs.duration is None) ^ (rhs.duration is None):
        return lhs.duration is None

    if len(rhs.lines) > len(lhs.lines):
        return True

    return rhs.duration > lhs.duration


def normalize_logs(llogs: List[LogStage]) -> List[LogStage]:
    normalized_logs = []
    buckets = {}
    for log in llogs:
        if log.name in buckets:
            if _is_worse(normalized_logs[buckets[log.name]], log):
                normalized_logs[buckets[log.name]] = log
        else:
            normalized_logs.append(log)
            buckets[log.name] = len(normalized_logs) - 1

    return normalized_logs


def count_levels(logs: Union[List[LogLine], LogStage]) -> Counter:
    if isinstance(logs, list):
        return Counter(log.level for log in logs)

    if isinstance(logs, LogStage):
        return count_levels(logs.lines)

    assert False, f"Type {type(logs)} is unsupported."


def find_and_parse(
    logs: Union[List[LogLine], LogStage], pattern: Union[str, type(re.compile(""))],
) -> List[Tuple[dict, str]]:
    if isinstance(pattern, str):
        pattern = re.compile(pattern, FLAGS)

    if isinstance(logs, list):
        found = []
        for log in logs:
            m = pattern.match(log.message)
            if m:
                found.append((m.groupdict(), log))
        return found

    if isinstance(logs, LogStage):
        return find_and_parse(logs.lines, pattern)

    assert False, f"Type {type(logs)} is unsupported."
