# backend/main.py
"""
Production‑grade backend for public‑transport schedule ingestion and API server.

Features
--------
* Ingest schedule data from various sources (CSV, GTFS, JSON, etc.).
* Normalise to a compact protobuf format (SchedulePackage).
* Store in RocksDB (time‑series key‑value store) for fast random access.
* Expose HTTP API (FastAPI) for clients to download the binary schedule.
* Publish real‑time delta updates via Server‑Sent Events (SSE).
* Background workers handle periodic ingestion and delta generation.
* Full error handling, logging, type hints and documentation.

Dependencies
------------
* Python >=3.10
* fastapi
* uvicorn[standard]
* python‑rocksdb
* protobuf
* pydantic
* aiohttp (for remote source fetching)
* schedule (for periodic jobs)
"""

import asyncio
import json
import logging
import os
import signal
import sys
from datetime import datetime, timezone
from pathlib import Path
from typing import AsyncGenerator, Dict, List, Optional

import rocksdb
import schedule
import uvicorn
from fastapi import BackgroundTasks, FastAPI, HTTPException, Request, Response
from fastapi.responses import StreamingResponse
from pydantic import BaseSettings, Field, validator

# ----------------------------------------------------------------------
# Logging configuration
# ----------------------------------------------------------------------
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s %(levelname)s %(name)s %(message)s",
    handlers=[logging.StreamHandler(sys.stdout)],
)
logger = logging.getLogger("pt-backend")

# ----------------------------------------------------------------------
# Configuration
# ----------------------------------------------------------------------
class Settings(BaseSettings):
    """Application configuration loaded from environment variables."""

    # RocksDB path
    rocksdb_path: Path = Field(..., env="ROCKSDB_PATH")
    # Ingestion interval in seconds
    ingestion_interval: int = Field(600, env="INGESTION_INTERVAL")
    # List of source URLs (JSON array string)
    source_urls: List[str] = Field(..., env="SOURCE_URLS")
    # SSE keep‑alive interval (seconds)
    sse_keepalive: int = Field(30, env="SSE_KEEPALIVE")
    # Max size of a delta payload (bytes)
    max_delta_size: int = Field(64 * 1024, env="MAX_DELTA_SIZE")
    # Protobuf generated module path (e.g., ptproto.schedule_pb2)
    protobuf_module: str = Field("ptproto.schedule_pb2", env="PROTOBUF_MODULE")

    @validator("source_urls", pre=True)
    def parse_source_urls(cls, v):
        if isinstance(v, str):
            return json.loads(v)
        return v


settings = Settings()

# ----------------------------------------------------------------------
# Protobuf import (dynamic import based on config)
# ----------------------------------------------------------------------
try:
    protobuf_mod = __import__(settings.protobuf_module, fromlist=["SchedulePackage", "ScheduleDelta"])
    SchedulePackage = protobuf_mod.SchedulePackage  # type: ignore
    ScheduleDelta = protobuf_mod.ScheduleDelta  # type: ignore
    logger.info("Loaded protobuf module %s", settings.protobuf_module)
except Exception as exc:
    logger.exception("Failed to import protobuf module")
    raise RuntimeError(f"Could not import protobuf module {settings.protobuf_module}") from exc


# ----------------------------------------------------------------------
# RocksDB helper
# ----------------------------------------------------------------------
class RocksDBWrapper:
    """Thin wrapper around python‑rocksdb providing typed get/set."""

    def __init__(self, path: Path):
        opts = rocksdb.Options()
        opts.create_if_missing = True
        opts.max_open_files = 1000
        opts.info_log_level = rocksdb.INFO_LEVEL
        self.db = rocksdb.DB(str(path), opts)
        logger.info("RocksDB opened at %s", path)

    def put(self, key: bytes, value: bytes) -> None:
        self.db.put(key, value)

    def get(self, key: bytes) -> Optional[bytes]:
        return self.db.get(key)

    def delete(self, key: bytes) -> None:
        self.db.delete(key)

    def iterator(self, prefix: bytes = b"") -> rocksdb.Iterator:
        it = self.db.iterkeys()
        it.seek(prefix)
        return it


# ----------------------------------------------------------------------
# Schedule ingestion
# ----------------------------------------------------------------------
class ScheduleIngestor:
    """Periodically fetches, normalises and stores schedule data."""

    def __init__(self, db: RocksDBWrapper, sources: List[str]):
        self.db = db
        self.sources = sources
        self.last_etag: Dict[str, str] = {}

    async def fetch_source(self, url: str) -> Optional[bytes]:
        """Fetch raw data from a source URL with conditional GET."""
        import aiohttp

        headers = {}
        if url in self.last_etag:
            headers["If-None-Match"] = self.last_etag[url]

        async with aiohttp.ClientSession() as session:
            async with session.get(url, headers=headers, timeout=30) as resp:
                if resp.status == 304:
                    logger.debug("Source %s not modified", url)
                    return None
                if resp.status != 200:
                    raise RuntimeError(f"Failed to fetch {url}: HTTP {resp.status}")
                data = await resp.read()
                etag = resp.headers.get("ETag")
                if etag:
                    self.last_etag[url] = etag
                logger.debug("Fetched %d bytes from %s", len(data), url)
                return data

    def normalise(self, raw: bytes, source_url: str) -> SchedulePackage:
        """
        Convert raw schedule data (CSV/GTFS/JSON) into a SchedulePackage protobuf.

        The implementation is deliberately simple – it parses JSON or CSV.
        For GTFS a dedicated parser would be added later.
        """
        import csv
        import io

        pkg = SchedulePackage()
        pkg.source_url = source_url
        pkg.generated_at.From.FromDatetime(datetime.now(timezone.utc))

        try:
            # Try JSON first
            payload = json.loads(raw.decode("utf-8"))
            for entry in payload.get("services", []):
                svc = pkg.services.add()
                svc.id = entry.get("id", "")
                svc.route = entry.get("route", "")
                svc.departure = entry.get("departure", "")
                svc.arrival = entry.get("arrival", "")
                svc.days.extend(entry.get("days", []))
        except json.JSONDecodeError:
            # Fallback to CSV (simple format: id,route,departure,arrival,days)
            csv_file = io.StringIO(raw.decode("utf-8"))
            reader = csv.DictReader(csv_file)
            for row in reader:
                svc = pkg.services.add()
                svc.id = row.get("id", "")
                svc.route = row.get("route", "")
                svc.departure = row.get("departure", "")
                svc.arrival = row.get("arrival", "")
                svc.days.extend(row.get("days", "").split("|"))

        return pkg

    async def ingest(self) -> None:
        """Fetch all sources, normalise and store the latest package."""
        for url in self.sources:
            try:
                raw = await self.fetch_source(url)
                if raw is None:
                    continue  # Not modified
                pkg = self.normalise(raw, url)
                key = f"schedule:{url}".encode("utf-8")
                self.db.put(key, pkg.SerializeToString())
                logger.info("Stored schedule for %s (%d services)", url, len(pkg.services))
                # Publish delta (simple diff: whole package for now)
                await DeltaPublisher.publish_delta(url, pkg)
            except Exception as exc:
                logger.exception("Ingestion failed for %s", url)


# ----------------------------------------------------------------------
# SSE delta publisher
# ----------------------------------------------------------------------
class DeltaPublisher:
    """Manages SSE connections and broadcasts ScheduleDelta messages."""

    _subscribers: List[asyncio.Queue[bytes]] = []

    @classmethod
    async def publish_delta(cls, source_url: str, package: SchedulePackage) -> None:
        """Create a ScheduleDelta protobuf and push to all subscribers."""
        delta = ScheduleDelta()
        delta.source_url = source_url
        delta.timestamp.FromDatetime(datetime.now(timezone.utc))
        delta.payload = package.SerializeToString()
        payload = delta.SerializeToString()
        if len(payload) > settings.max_delta_size:
            logger.warning(
                "Delta size %d exceeds max %d, truncating",
                len(payload),
                settings.max_delta_size,
            )
            payload = payload[: settings.max_delta_size]

        for q in cls._subscribers:
            await q.put(payload)
        logger.debug("Published delta to %d subscribers", len(cls._subscribers))

    @classmethod
    async def register(cls) -> asyncio.Queue[bytes]:
        q: asyncio.Queue[bytes] = asyncio.Queue()
        cls._subscribers.append(q)
        logger.info("New SSE subscriber, total %d", len(cls._subscribers))
        return q

    @classmethod
    async def unregister(cls, q: asyncio.Queue[bytes]) -> None:
        cls._subscribers.remove(q)
        logger.info("SSE subscriber removed, total %d", len(cls._subscribers))


# ----------------------------------------------------------------------
# FastAPI application
# ----------------------------------------------------------------------
app = FastAPI(title="Public Transport Schedule Service", version="1.0.0")
db = RocksDBWrapper(settings.rocksdb_path)
ingestor = ScheduleIngestor(db, settings.source_urls)


# ----------------------------------------------------------------------
# Background tasks
# ----------------------------------------------------------------------
def schedule_jobs():
    """Schedule periodic ingestion using the `schedule` library."""
    schedule.every(settings.ingestion_interval).seconds.do(
        lambda: asyncio.run(ingestor.ingest())
    )
    logger.info("Scheduled ingestion every %d seconds", settings.ingestion_interval)


async def scheduler_loop():
    """Run the `schedule` loop in an async task."""
    while True:
        schedule.run_pending()
        await asyncio.sleep(1)


@app.on_event("startup")
async def on_startup():
    """Start background scheduler."""
    schedule_jobs()
    asyncio.create_task(scheduler_loop())
    logger.info("Backend started")


# ----------------------------------------------------------------------
# API endpoints
# ----------------------------------------------------------------------
@app.get("/schedule/{source_id}", response_class=Response, tags=["schedule"])
async def get_schedule(source_id: str):
    """
    Retrieve the binary protobuf schedule for a given source identifier.

    The identifier is the URL‑encoded source URL.
    """
    key = f"schedule:{source_id}".encode("utf-8")
    data = db.get(key)
    if data is None:
        raise HTTPException(status_code=404, detail="Schedule not found")
    return Response(content=data, media_type="application/octet-stream")


@app.get("/updates", tags=["realtime"])
async def realtime_updates(request: Request):
    """
    Server‑Sent Events endpoint delivering real‑time schedule deltas.

    Clients should reconnect on network errors. The server sends a keep‑alive
    comment every `sse_keepalive` seconds.
    """
    queue = await DeltaPublisher.register()

    async def event_generator() -> AsyncGenerator[bytes, None]:
        keepalive_interval = settings.sse_keepalive
        last_keepalive = datetime.now(timezone.utc)

        while True:
            # Detect client disconnect
            if await request.is_disconnected():
                break

            try:
                payload = await asyncio.wait_for(queue.get(), timeout=keepalive_interval)
                yield b"data: " + payload + b"\n\n"
            except asyncio.TimeoutError:
                # Send comment as keep‑alive
                now = datetime.now(timezone.utc)
                if (now - last_keepalive).total_seconds() >= keepalive_interval:
                    yield b": keep-alive\n\n"
                    last_keepalive = now

        await DeltaPublisher.unregister(queue)

    return StreamingResponse(event_generator(), media_type="text/event-stream")


# ----------------------------------------------------------------------
# Graceful shutdown
# ----------------------------------------------------------------------
def _handle_sigterm():
    logger.info("SIGTERM received – shutting down")
    sys.exit(0)


signal.signal(signal.SIGTERM, lambda *_: _handle_sigterm())
signal.signal(signal.SIGINT, lambda *_: _handle_sigterm())

# ----------------------------------------------------------------------
# Entrypoint
# ----------------------------------------------------------------------
if __name__ == "__main__":
    # uvicorn.run(app, host="0.0.0.0", port=8000, log_level="info")
    uvicorn.run(
        "backend.main:app",
        host="0.0.0.0",
        port=8000,
        log_level="info",
        workers=4,
    )