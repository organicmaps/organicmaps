# client/transport_decoder.py
"""Transport schedule binary decoder.

The module parses a compact binary schedule blob (produced by the
backend) and builds in‑memory data structures used by the routing
engine.  The binary format is deliberately simple to keep the decoder
lightweight and dependency‑free:

+----------------------+-------------------+--------------------------+
| Field                | Type              | Description              |
+----------------------+-------------------+--------------------------+
| magic                | 4 bytes (ascii)   | b'PTBL' – file identifier|
| version              | uint8             | format version (currently 1)|
| trip_count           | uint32 (LE)       | number of trips          |
| stop_count           | uint32 (LE)       | number of distinct stops|
| ---- per‑trip block ----                                      |
|   trip_id            | uint64 (LE)       | unique trip identifier   |
|   start_time         | uint32 (LE)       | seconds from midnight    |
|   end_time           | uint32 (LE)       | seconds from midnight    |
|   stop_sequence_len  | uint16 (LE)       | number of stop‑time entries|
|   per‑stop‑time entry (repeated stop_sequence_len)          |
|       stop_id        | uint32 (LE)       | stop identifier          |
|       arrival_offset | uint32 (LE)       | seconds after start_time |
|       departure_offset| uint32 (LE)      | seconds after start_time |
| ---- per‑stop block ----                                      |
|   stop_id            | uint32 (LE)       | stop identifier          |
|   latitude          | int32  (LE)       | micro‑degrees (1e‑6)     |
|   longitude         | int32  (LE)       | micro‑degrees (1e‑6)     |
|   name_len          | uint8             | length of UTF‑8 name      |
|   name (UTF‑8)      | variable          | stop name                |
+----------------------+-------------------+--------------------------+

The decoder validates the magic header, version, and internal
consistency (e.g. referenced stop ids must exist).  All errors are
raised as :class:`TransportDecoderError` with a clear message.

Typical usage
-------------
>>> from client.transport_decoder import TransportDecoder
>>> decoder = TransportDecoder()
>>> schedule = decoder.decode(blob_bytes)
>>> schedule.trips[0].stop_times[0].stop.name
"""

from __future__ import annotations

import logging
import struct
from dataclasses import dataclass, field
from typing import Dict, List, Sequence

# --------------------------------------------------------------------------- #
# Logging configuration (the application may override this configuration)
# --------------------------------------------------------------------------- #
logger = logging.getLogger(__name__)
if not logger.handlers:
    # Default simple console handler – can be re‑configured by the caller.
    handler = logging.StreamHandler()
    formatter = logging.Formatter(
        fmt="%(asctime)s %(levelname)s %(name)s – %(message)s",
        datefmt="%Y-%m-%d %H:%M:%S",
    )
    handler.setFormatter(formatter)
    logger.addHandler(handler)
    logger.setLevel(logging.INFO)


# --------------------------------------------------------------------------- #
# Public data structures
# --------------------------------------------------------------------------- #
@dataclass(frozen=True, slots=True)
class Stop:
    """A physical stop (station, bus stop, etc.)."""

    stop_id: int
    latitude: float   # decimal degrees
    longitude: float  # decimal degrees
    name: str


@dataclass(frozen=True, slots=True)
class StopTime:
    """A stop occurrence within a trip."""

    stop: Stop
    arrival: int   # seconds from trip start
    departure: int  # seconds from trip start


@dataclass(frozen=True, slots=True)
class Trip:
    """A single public‑transport trip."""

    trip_id: int
    start_time: int   # seconds from midnight
    end_time: int     # seconds from midnight
    stop_times: Sequence[StopTime] = field(default_factory=tuple)


@dataclass(frozen=True, slots=True)
class Schedule:
    """Container for all decoded schedule data."""

    trips: List[Trip] = field(default_factory=list)
    stops: Dict[int, Stop] = field(default_factory=dict)


# --------------------------------------------------------------------------- #
# Exceptions
# --------------------------------------------------------------------------- #
class TransportDecoderError(RuntimeError):
    """Base class for all decoding errors."""


# --------------------------------------------------------------------------- #
# Decoder implementation
# --------------------------------------------------------------------------- #
class TransportDecoder:
    """Parse a binary transport schedule blob.

    The decoder is deliberately stateless – it can be reused for
    multiple blobs without side effects.
    """

    # Binary constants
    _MAGIC = b"PTBL"
    _HEADER_FMT = "<4sBII"          # magic, version, trip_cnt, stop_cnt
    _TRIP_HEADER_FMT = "<QIIH"      # trip_id, start, end, stop_seq_len
    _STOP_TIME_FMT = "<III"          # stop_id, arrival_offset, departure_offset
    _STOP_HEADER_FMT = "<IiiB"       # stop_id, lat, lon, name_len

    def __init__(self) -> None:
        """Create a new decoder instance."""
        logger.debug("TransportDecoder instantiated")

    # --------------------------------------------------------------------- #
    # Public API
    # --------------------------------------------------------------------- #
    def decode(self, data: bytes) -> Schedule:
        """Decode the binary schedule blob.

        Parameters
        ----------
        data:
            Raw binary payload received from the backend.

        Returns
        -------
        Schedule
            Fully populated schedule object.

        Raises
        ------
        TransportDecoderError
            If the blob is malformed or inconsistent.
        """
        logger.info("Starting schedule decode – %d bytes", len(data))
        offset = 0

        # ---- Header ------------------------------------------------------ #
        try:
            magic, version, trip_cnt, stop_cnt = struct.unpack_from(
                self._HEADER_FMT, data, offset
            )
            offset += struct.calcsize(self._HEADER_FMT)
        except struct.error as exc:
            raise TransportDecoderError("Failed to unpack header") from exc

        logger.debug(
            "Header: magic=%s version=%d trips=%d stops=%d",
            magic,
            version,
            trip_cnt,
            stop_cnt,
        )
        if magic != self._MAGIC:
            raise TransportDecoderError(
                f"Invalid magic header: expected {self._MAGIC!r}, got {magic!r}"
            )
        if version != 1:
            raise TransportDecoderError(
                f"Unsupported version {version}; only version 1 is supported"
            )

        # ---- Stops (reference data) -------------------------------------- #
        stops: Dict[int, Stop] = {}
        for i in range(stop_cnt):
            stop, offset = self._decode_stop(data, offset)
            if stop.stop_id in stops:
                raise TransportDecoderError(
                    f"Duplicate stop_id {stop.stop_id} at index {i}"
                )
            stops[stop.stop_id] = stop
            logger.debug("Decoded stop %d: %s", i, stop)

        # ---- Trips ------------------------------------------------------- #
        trips: List[Trip] = []
        for i in range(trip_cnt):
            trip, offset = self._decode_trip(data, offset, stops)
            trips.append(trip)
            logger.debug("Decoded trip %d: %s", i, trip)

        # Ensure we consumed the whole blob
        if offset != len(data):
            logger.warning(
                "Trailing data detected: %d unused bytes after offset %d",
                len(data) - offset,
                offset,
            )

        schedule = Schedule(trips=trips, stops=stops)
        logger.info(
            "Decoding finished – %d trips, %d stops", len(trips), len(stops)
        )
        return schedule

    # --------------------------------------------------------------------- #
    # Internal helpers
    # --------------------------------------------------------------------- #
    def _decode_stop(self, data: bytes, offset: int) -> tuple[Stop, int]:
        """Decode a single stop entry."""
        try:
            stop_id, lat_raw, lon_raw, name_len = struct.unpack_from(
                self._STOP_HEADER_FMT, data, offset
            )
            offset += struct.calcsize(self._STOP_HEADER_FMT)
        except struct.error as exc:
            raise TransportDecoderError("Failed to unpack stop header") from exc

        # Name is UTF‑8 encoded, length is given by name_len
        name_bytes = data[offset : offset + name_len]
        offset += name_len
        try:
            name = name_bytes.decode("utf-8")
        except UnicodeDecodeError as exc:
            raise TransportDecoderError(
                f"Invalid UTF‑8 name for stop_id {stop_id}"
            ) from exc

        latitude = lat_raw / 1_000_000.0
        longitude = lon_raw / 1_000_000.0

        stop = Stop(
            stop_id=stop_id,
            latitude=latitude,
            longitude=longitude,
            name=name,
        )
        return stop, offset

    def _decode_trip(
        self,
        data: bytes,
        offset: int,
        stops: Dict[int, Stop],
    ) -> tuple[Trip, int]:
        """Decode a single trip entry, linking stop times to Stop objects."""
        try:
            trip_id, start_time, end_time, seq_len = struct.unpack_from(
                self._TRIP_HEADER_FMT, data, offset
            )
            offset += struct.calcsize(self._TRIP_HEADER_FMT)
        except struct.error as exc:
            raise TransportDecoderError("Failed to unpack trip header") from exc

        if start_time > end_time:
            raise TransportDecoderError(
                f"Trip {trip_id} start_time > end_time ({start_time} > {end_time})"
            )

        stop_times: List[StopTime] = []
        for i in range(seq_len):
            try:
                stop_id, arrival_off, depart_off = struct.unpack_from(
                    self._STOP_TIME_FMT, data, offset
                )
                offset += struct.calcsize(self._STOP_TIME_FMT)
            except struct.error as exc:
                raise TransportDecoderError(
                    f"Failed to unpack stop_time #{i} for trip {trip_id}"
                ) from exc

            if stop_id not in stops:
                raise TransportDecoderError(
                    f"Trip {trip_id} references unknown stop_id {stop_id}"
                )
            stop = stops[stop_id]

            if arrival_off > depart_off:
                raise TransportDecoderError(
                    f"Trip {trip_id} stop {stop_id} arrival > departure"
                )

            stop_time = StopTime(
                stop=stop,
                arrival=arrival_off,
                departure=depart_off,
            )
            stop_times.append(stop_time)

        trip = Trip(
            trip_id=trip_id,
            start_time=start_time,
            end_time=end_time,
            stop_times=tuple(stop_times),
        )
        return trip, offset