python
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
import logging.config
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
# Logging configuration (enhanced)
# ----------------------------------------------------------------------
LOG_LEVEL = os.getenv("LOG_LEVEL", "INFO").upper()
LOG_FORMAT = "%(asctime)s %(levelname)s %(name)s %(message)s"
LOG_DATEFMT = "%Y-%m-%d %H:%M:%S"

LOGGING_CONFIG = {
    "version": 1,
    "disable_existing_loggers": False,
    "formatters": {
        "standard": {"format": LOG_FORMAT, "datefmt": LOG_DATEFMT},
    },
    "handlers": {
        "console": {
            "class": "logging.StreamHandler",
            "formatter": "standard",
            "stream": sys.stdout,
        },
        "file": {
            "class": "logging.handlers.RotatingFileHandler",
            "formatter": "standard",
            "filename": "backend.log",
            "maxBytes": 10 * 1024 * 1024,  # 10 MiB
            "backupCount": 5,
            "encoding": "utf-8",
        },
    },
    "root": {"handlers": ["console", "file"], "level": LOG_LEVEL},
}
logging.config.dictConfig(LOGGING_CONFIG)
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
        pkg.generated_at.FromDatetime(datetime.now(timezone.utc))

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
    """Publishes schedule deltas via Server‑Sent Events."""

    _subscribers: List[asyncio.Queue] = []

    @classmethod
    async def publish_delta(cls, source_url: str, pkg: SchedulePackage) -> None:
        """Serialize a delta and push it to all connected SSE clients."""
        delta = ScheduleDelta()
        delta.source_url = source_url
        delta.generated_at.FromDatetime(datetime.now(timezone.utc))
        # For simplicity we send the whole package as a delta payload.
        delta.payload = pkg.SerializeToString()
        payload = delta.SerializeToString()

        for q in cls._subscribers:
            await q.put(payload)
        logger.debug("Published delta for %s to %d subscribers", source_url, len(cls._subscribers))

    @classmethod
    async def subscribe(cls) -> asyncio.Queue:
        """Create a new subscription queue for an SSE client."""
        q = asyncio.Queue()
        cls._subscribers.append(q)
        logger.debug("New SSE subscriber added (total=%d)", len(cls._subscribers))
        return q

    @classmethod
    def unsubscribe(cls, q: asyncio.Queue) -> None:
        """Remove a subscription queue."""
        cls._subscribers.remove(q)
        logger.debug("SSE subscriber removed (total=%d)", len(cls._subscribers))


# ----------------------------------------------------------------------
# FastAPI application
# ----------------------------------------------------------------------
app = FastAPI(title="Public‑Transport Schedule API", version="1.0.0")
db_wrapper = RocksDBWrapper(settings.rocksdb_path)
ingestor = ScheduleIngestor(db_wrapper, settings.source_urls)

# ----------------------------------------------------------------------
# Background tasks & graceful shutdown handling
# ----------------------------------------------------------------------
shutdown_event = asyncio.Event()


def _handle_signal(sig: signal.Signals):
    """Signal handler that triggers graceful shutdown."""
    logger.info("Received signal %s – initiating graceful shutdown", sig.name)
    shutdown_event.set()


for s in (signal.SIGINT, signal.SIGTERM):
    signal.signal(s, lambda signum, frame, sig=s: _handle_signal(sig))


async def _periodic_ingest():
    """Run ingestion at the configured interval until shutdown."""
    while not shutdown_event.is_set():
        try:
            await ingestor.ingest()
        except Exception as exc:
            logger.exception("Unexpected error during ingestion")
        # Wait for the next interval or early exit if shutdown is requested
        try:
            await asyncio.wait_for(shutdown_event.wait(), timeout=settings.ingestion_interval)
        except asyncio.TimeoutError:
            continue


@app.on_event("startup")
async def on_startup():
    """Start background ingestion task."""
    logger.info("Application startup – launching ingestion loop")
    app.state.ingest_task = asyncio.create_task(_periodic_ingest())


@app.on_event("shutdown")
async def on_shutdown():
    """Signal background task to stop and wait for its completion."""
    logger.info("Application shutdown – waiting for background tasks")
    shutdown_event.set()
    ingest_task: asyncio.Task = app.state.ingest_task
    ingest_task.cancel()
    try:
        await ingest_task
    except asyncio.CancelledError:
        pass
    logger.info("Background ingestion task terminated")


# ----------------------------------------------------------------------
# API endpoints
# ----------------------------------------------------------------------
@app.get("/schedule/{source_url:path}")
async def get_schedule(source_url: str):
    """Return the latest SchedulePackage for a given source."""
    key = f"schedule:{source_url}".encode("utf-8")
    data = db_wrapper.get(key)
    if data is None:
        raise HTTPException(status_code=404, detail="Schedule not found")
    return Response(content=data, media_type="application/octet-stream")


@app.get("/sse/delta")
async def sse_delta(request: Request):
    """Server‑Sent Events endpoint for real‑time schedule deltas."""
    queue = await DeltaPublisher.subscribe()

    async def event_generator():
        try:
            while True:
                # If client disconnects, request.is_disconnected() becomes True
                if await request.is_disconnected():
                    break
                try:
                    payload = await asyncio.wait_for(queue.get(), timeout=settings.sse_keepalive)
                    yield f"data: {payload.hex()}\n\n"
                except asyncio.TimeoutError:
                    # Send a keep‑alive comment to prevent timeout
                    yield ": keep-alive\n\n"
        finally:
            DeltaPublisher.unsubscribe(queue)

    return StreamingResponse(event_generator(), media_type="text/event-stream")


# ----------------------------------------------------------------------
# Entrypoint
# ----------------------------------------------------------------------
def main() -> None:
    """Run the FastAPI server with graceful shutdown support."""
    config = uvicorn.Config(
        app,
        host="0.0.0.0",
        port=8000,
        log_level=LOG_LEVEL.lower(),
        timeout_keep_alive=15,
    )
    server = uvicorn.Server(config)

    # Run uvicorn in the event loop so we can react to signals
    loop = asyncio.get_event_loop()
    loop.create_task(server.serve())
    try:
        loop.run_until_complete(shutdown_event.wait())
    finally:
        # Ensure uvicorn shuts down cleanly
        logger.info("Shutting down Uvicorn server")
        loop.run_until_complete(server.shutdown())
        loop.stop()


if __name__ == "__main__":
    main()