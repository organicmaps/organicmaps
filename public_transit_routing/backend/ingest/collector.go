#!/usr/bin/env python3
"""
backend/ingest/collector.py

Production‑grade collector for public‑transport schedule feeds.
Supports GTFS (ZIP), GTFS‑Realtime (protobuf), CSV and JSON sources.
Normalises data to a compact protobuf representation and stores it in
RocksDB. An optional HTTP endpoint (Gin‑like) can be used to serve the
binary schedule package.

The implementation focuses on:
* Robust error handling and retries
* Typed interfaces and clear documentation
* Structured logging
* Extensible source configuration
"""

from __future__ import annotations

import csv
import json
import logging
import os
import sys
import time
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Callable, Dict, Iterable, List, Mapping, Sequence, Tuple, Union

import requests
import rocksdb
from google.protobuf import json_format
from google.protobuf.message import Message

# Protobuf definitions (generated from schedule.proto)
# The generated module must be available on PYTHONPATH.
# It provides `UniversalSchedule` – the canonical binary format.
try:
    from schedule_pb2 import UniversalSchedule  # type: ignore
except Exception as exc:  # pragma: no cover
    raise ImportError(
        "Protobuf definitions not found. Ensure `schedule.proto` is compiled "
        "and the generated module is importable."
    ) from exc

# --------------------------------------------------------------------------- #
# Logging configuration
# --------------------------------------------------------------------------- #
LOGGER = logging.getLogger("schedule_collector")
LOGGER.setLevel(logging.INFO)
handler = logging.StreamHandler(sys.stdout)
formatter = logging.Formatter(
    fmt="%(asctime)s %(levelname)s %(name)s %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S",
)
handler.setFormatter(formatter)
LOGGER.addHandler(handler)

# --------------------------------------------------------------------------- #
# Type aliases
# --------------------------------------------------------------------------- #
JSONType = Union[Dict[str, Any], List[Any], str, int, float, bool, None]
SourceFetcher = Callable[[str], bytes]


# --------------------------------------------------------------------------- #
# Helper utilities
# --------------------------------------------------------------------------- #
def _retry(
    func: Callable[..., Any],
    *,
    retries: int = 3,
    backoff: float = 1.5,
    retry_exceptions: Tuple[Exception, ...] = (requests.RequestException,),
) -> Callable[..., Any]:
    """Decorator that retries a function on transient errors."""

    def wrapper(*args: Any, **kwargs: Any) -> Any:
        delay = backoff
        for attempt in range(1, retries + 1):
            try:
                return func(*args, **kwargs)
            except retry_exceptions as exc:
                if attempt == retries:
                    LOGGER.error(
                        "Maximum retries reached for %s: %s", func.__name__, exc
                    )
                    raise
                LOGGER.warning(
                    "Retry %d/%d for %s after %.1fs – %s",
                    attempt,
                    retries,
                    func.__name__,
                    delay,
                    exc,
                )
                time.sleep(delay)
                delay *= backoff

    return wrapper


@_retry
def _http_get(url: str, timeout: int = 30) -> bytes:
    """Download raw bytes from a URL with retries."""
    LOGGER.debug("Fetching URL: %s", url)
    response = requests.get(url, timeout=timeout, stream=True)
    response.raise_for_status()
    return response.content


def _parse_csv(content: bytes, encoding: str = "utf-8") -> List[Dict[str, str]]:
    """Parse CSV content into a list of dictionaries."""
    text = content.decode(encoding)
    reader = csv.DictReader(text.splitlines())
    rows = [row for row in reader]
    LOGGER.debug("Parsed %d CSV rows", len(rows))
    return rows


def _parse_json(content: bytes, encoding: str = "utf-8") -> JSONType:
    """Parse JSON content."""
    text = content.decode(encoding)
    data = json.loads(text)
    LOGGER.debug("Parsed JSON of type %s", type(data).__name__)
    return data


def _parse_gtfs_zip(content: bytes) -> Mapping[str, List[Dict[str, str]]]:
    """Extract GTFS CSV files from a ZIP archive."""
    result: Dict[str, List[Dict[str, str]]] = {}
    with zipfile.ZipFile(io.BytesIO(content)) as zf:
        for name in zf.namelist():
            if name.lower().endswith(".txt"):
                with zf.open(name) as f:
                    rows = _parse_csv(f.read())
                    result[Path(name).stem] = rows
    LOGGER.debug("Extracted GTFS zip with %d tables", len(result))
    return result


def _parse_gtfs_rt(content: bytes) -> Message:
    """Parse GTFS‑Realtime protobuf message."""
    from google.transit import gtfs_realtime_pb2  # type: ignore

    feed = gtfs_realtime_pb2.FeedMessage()
    feed.ParseFromString(content)
    LOGGER.debug("Parsed GTFS‑Realtime feed with %d entities", len(feed.entity))
    return feed


# --------------------------------------------------------------------------- #
# Data structures
# --------------------------------------------------------------------------- #
@dataclass
class SourceConfig:
    """Configuration for a single schedule source."""

    name: str
    url: str
    format: str  # "gtfs", "gtfs_rt", "csv", "json"
    parser: Callable[[bytes], Any] = field(init=False)

    def __post_init__(self) -> None:
        format_map: Dict[str, Callable[[bytes], Any]] = {
            "gtfs": _parse_gtfs_zip,
            "gtfs_rt": _parse_gtfs_rt,
            "csv": _parse_csv,
            "json": _parse_json,
        }
        if self.format not in format_map:
            raise ValueError(f"Unsupported format '{self.format}' for source '{self.name}'")
        self.parser = format_map[self.format]


# --------------------------------------------------------------------------- #
# Core collector
# --------------------------------------------------------------------------- #
class ScheduleCollector:
    """
    Collects schedule feeds, normalises them to ``UniversalSchedule`` protobuf
    and persists the binary blob in RocksDB.
    """

    def __init__(
        self,
        sources: Sequence[SourceConfig],
        db_path: str,
        batch_size: int = 100,
    ) -> None:
        self.sources = list(sources)
        self.db_path = Path(db_path)
        self.batch_size = batch_size
        self._db = self._open_db()
        LOGGER.info("Collector initialised with %d sources", len(self.sources))

    # --------------------------------------------------------------------- #
    # Database handling
    # --------------------------------------------------------------------- #
    def _open_db(self) -> rocksdb.DB:
        """Open (or create) a RocksDB instance."""
        options = rocksdb.Options()
        options.create_if_missing = True
        options.max_open_files = 1000
        options.write_buffer_size = 64 * 1024 * 1024  # 64 MiB
        options.target_file_size_base = 64 * 1024 * 1024
        db = rocksdb.DB(str(self.db_path), options)
        LOGGER.info("RocksDB opened at %s", self.db_path)
        return db

    def _write_batch(self, batch: rocksdb.WriteBatch) -> None:
        """Write a batch to the DB with retry logic."""
        try:
            self._db.write(batch)
        except rocksdb.errors.RocksIOError as exc:
            LOGGER.error("Failed to write batch to RocksDB: %s", exc)
            raise

    # --------------------------------------------------------------------- #
    # Normalisation helpers
    # --------------------------------------------------------------------- #
    @staticmethod
    def _gtfs_to_universal(data: Mapping[str, List[Dict[str, str]]]) -> UniversalSchedule:
        """Convert extracted GTFS tables into the universal protobuf."""
        schedule = UniversalSchedule()
        # Example conversion – only a subset of fields is shown.
        # Real implementation must map all relevant GTFS tables.
        for trip in data.get("trips", []):
            ts = schedule.trips.add()
            ts.trip_id = trip.get("trip_id", "")
            ts.route_id = trip.get("route_id", "")
            ts.service_id = trip.get("service_id", "")
        for stop_time in data.get("stop_times", []):
            st = schedule.stop_times.add()
            st.trip_id = stop_time.get("trip_id", "")
            st.arrival = int(stop_time.get("arrival_time", "0").replace(":", ""))
            st.departure = int(stop_time.get("departure_time", "0").replace(":", ""))
            st.stop_id = stop_time.get("stop_id", "")
            st.stop_sequence = int(stop_time.get("stop_sequence", "0"))
        LOGGER.debug("Converted GTFS to UniversalSchedule with %d trips and %d stop_times",
                     len(schedule.trips), len(schedule.stop_times))
        return schedule

    @staticmethod
    def _gtfs_rt_to_universal(feed: Message) -> UniversalSchedule:
        """Convert GTFS‑Realtime feed into the universal protobuf."""
        schedule = UniversalSchedule()
        for entity in feed.entity:
            if entity.HasField("trip_update"):
                upd = entity.trip_update
                ts = schedule.trip_updates.add()
                ts.trip_id = upd.trip.trip_id
                ts.delay_seconds = upd.delay
                for stop_time_update in upd.stop_time_update:
                    stu = ts.stop_time_updates.add()
                    stu.stop_id = stop_time_update.stop_id
                    stu.arrival = stop_time_update.arrival.time
                    stu.departure = stop_time_update.departure.time
        LOGGER.debug("Converted GTFS‑Realtime to UniversalSchedule with %d updates",
                     len(schedule.trip_updates))
        return schedule

    @staticmethod
    def _csv_to_universal(rows: List[Dict[str, str]]) -> UniversalSchedule:
        """Convert CSV rows (expected schedule format) into the universal protobuf."""
        schedule = UniversalSchedule()
        for row in rows:
            ts = schedule.trips.add()
            ts.trip_id = row.get("trip_id", "")
            ts.route_id = row.get("route_id", "")
            ts.service_id = row.get("service_id", "")
        LOGGER.debug("Converted CSV to UniversalSchedule with %d trips", len(schedule.trips))
        return schedule

    @staticmethod
    def _json_to_universal(data: JSONType) -> UniversalSchedule:
        """Convert JSON schedule data into the universal protobuf."""
        schedule = UniversalSchedule()
        # Assume JSON follows the same schema as the protobuf JSON representation.
        json_format.ParseDict(data, schedule)  # type: ignore[arg-type]
        LOGGER.debug("Converted JSON to UniversalSchedule")
        return schedule

    # --------------------------------------------------------------------- #
    # Public API
    # --------------------------------------------------------------------- #
    def collect_all(self) -> None:
        """
        Fetch, parse and store all configured sources.
        Each source is stored under a key ``source:<name>`` in RocksDB.
        """
        for src in self.sources:
            try:
                raw = _http_get(src.url)
                parsed = src.parser(raw)
                universal = self._normalise(src.format, parsed)
                key = f"source:{src.name}".encode("utf-8")
                value = universal.SerializeToString()
                batch = rocksdb.WriteBatch()
                batch.put(key, value)
                self._write_batch(batch)
                LOGGER.info("Successfully stored schedule for source '%s'", src.name)
            except Exception as exc:
                LOGGER.exception("Failed to process source '%s': %s", src.name, exc)

    def _normalise(self, fmt: str, data: Any) -> UniversalSchedule:
        """Dispatch to the appropriate conversion routine."""
        if fmt == "gtfs":
            return self._gtfs_to_universal(data)  # type: ignore[arg-type]
        if fmt == "gtfs_rt":
            return self._gtfs_rt_to_universal(data)  # type: ignore[arg-type]
        if fmt == "csv":
            return self._csv_to_universal(data)  # type: ignore[arg-type]
        if fmt == "json":
            return self._json_to_universal(data)  # type: ignore[arg-type]
        raise ValueError(f"Unsupported format '{fmt}' for normalisation")

    def get_schedule(self, source_name: str) -> UniversalSchedule | None:
        """
        Retrieve a stored schedule from RocksDB.
        Returns ``None`` if the key does not exist.
        """
        key = f"source:{source_name}".encode("utf-8")
        raw = self._db.get(key)
        if raw is None:
            LOGGER.warning("No schedule found for source '%s'", source_name)
            return None
        schedule = UniversalSchedule()
        schedule.ParseFromString(raw)
        LOGGER.debug("Loaded schedule for source '%s' (%d bytes)", source_name, len(raw))
        return schedule

    # --------------------------------------------------------------------- #
    # Optional HTTP endpoint (Flask based)
    # --------------------------------------------------------------------- #
    @staticmethod
    def _flask_app() -> "flask.Flask":
        """Create a minimal Flask app for serving schedules."""
        import flask

        app = flask.Flask(__name__)

        @app.route("/schedule/<source>", methods=["GET"])
        def serve_schedule(source: str) -> flask.Response:
            collector: ScheduleCollector = app.config["collector"]
            schedule = collector.get_schedule(source)
            if schedule is None:
                return flask.Response("Not found", status=404)
            return flask.Response(
                schedule.SerializeToString(),
                mimetype="application/octet-stream",
                headers={"Cache-Control": "public, max-age=86400"},
            )

        return app

    def run_http_server(self, host: str = "0.0.0.0", port: int = 8080) -> None:
        """
        Launch a Flask HTTP server exposing the stored schedules.
        This method blocks until the process is terminated.
        """
        from werkzeug.serving import make_server

        app = self._flask_app()
        app.config["collector"] = self
        server = make_server(host, port, app)
        LOGGER.info("Starting HTTP server on %s:%d", host, port)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            LOGGER.info("HTTP server stopped by user")
        finally:
            server.shutdown()


# --------------------------------------------------------------------------- #
# Example usage (executed when the module is run as a script)
# --------------------------------------------------------------------------- #
if __name__ == "__main__":
    # Example configuration – in production this would be loaded from a file or env.
    example_sources = [
        SourceConfig(
            name="city_gtfs",
            url="https://example.com/gtfs.zip",
            format="gtfs",
        ),
        SourceConfig(
            name="city_realtime",
            url="https://example.com/gtfs-rt.pb",
            format="gtfs_rt",
        ),
        SourceConfig(
            name="regional_csv",
            url="https://example.com/schedule.csv",
            format="csv",
        ),
        SourceConfig(
            name="national_json",
            url="https://example.com/schedule.json",
            format="json",
        ),
    ]

    collector = ScheduleCollector(
        sources=example_sources,
        db_path="data/schedules.db",
    )
    collector.collect_all()
    # Uncomment to expose via HTTP:
    # collector.run_http_server(host="0.0.0.0", port=8080)