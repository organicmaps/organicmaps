#!/usr/bin/env python3
"""
Rider Live Location Tracking Server for Organic Maps.

Zero-dependency: only Python 3.10+ stdlib (http.server, json, sqlite3).
Deploy on any cloud VSI (EC2, DigitalOcean, GCP, Linode, etc.):
    RIDER_USER=alice RIDER_PASS=hunter2 python3 rider_server.py

Endpoints:
    GET  /health                                       -> liveness (public)
    POST /api/location {code,name?,lat,lon,speed?,bearing?}  -> upsert rider position
    GET  /api/location?code=XXX                        -> single rider position
    POST /api/locations/batch {codes:[...]}            -> multi-rider lookup

Resilience:
    * Per-request handlers are wrapped so a single bad request never kills
      the process — it responds 500 and keeps serving.
    * ThreadingHTTPServer isolates requests so one slow / broken client
      cannot stall the others.
    * An outer supervisor loop restarts the HTTP server if it dies for any
      reason (unbound exception, socket loss, etc.) with exponential backoff
      capped at 30 s. SIGTERM / SIGINT exit cleanly.
    * SQLite is in-memory and lives in the parent process, so in-process
      restarts preserve state; only a full process kill loses it.
"""

import base64
import hmac
import json
import os
import signal
import sqlite3
import sys
import time
import traceback
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

HOST = os.environ.get("RIDER_HOST", "0.0.0.0")
PORT = int(os.environ.get("RIDER_PORT", "8080"))
LOCATION_TTL_SECONDS = int(os.environ.get("RIDER_TTL", "600"))  # 10 min default

# HTTP Basic Auth. Set via env vars. If either is empty, auth is disabled.
AUTH_USER = os.environ.get("RIDER_USER", "rider")
AUTH_PASS = os.environ.get("RIDER_PASS", "changeme")
AUTH_ENABLED = bool(AUTH_USER and AUTH_PASS)

# Supervisor backoff: start at 1 s, double on each failure, cap at 30 s.
RESTART_BACKOFF_START = 1.0
RESTART_BACKOFF_MAX = 30.0

# Set by the signal handler so the supervisor loop knows to stop.
_shutdown_requested = False


def init_db():
    conn = sqlite3.connect(":memory:", check_same_thread=False)
    with conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS rider_locations (
                code TEXT PRIMARY KEY,
                name TEXT DEFAULT '',
                lat REAL NOT NULL,
                lon REAL NOT NULL,
                speed REAL DEFAULT 0,
                bearing REAL DEFAULT 0,
                updated_at INTEGER NOT NULL
            )
        """)
    return conn


db_conn = init_db()


class RiderRequestHandler(BaseHTTPRequestHandler):
    # ---- response helpers -------------------------------------------------

    def send_json(self, status_code, data):
        body = json.dumps(data).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def _check_auth(self):
        if not AUTH_ENABLED:
            return True
        header = self.headers.get("Authorization", "")
        if not header.startswith("Basic "):
            return False
        try:
            decoded = base64.b64decode(header[6:].strip()).decode("utf-8")
            user, _, pwd = decoded.partition(":")
        except (ValueError, UnicodeDecodeError):
            return False
        return hmac.compare_digest(user, AUTH_USER) and hmac.compare_digest(pwd, AUTH_PASS)

    def _require_auth(self):
        body = json.dumps({"error": "Unauthorized"}).encode("utf-8")
        self.send_response(401)
        self.send_header("WWW-Authenticate", 'Basic realm="RiderTracking"')
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _send_500(self, exc):
        # Never let internal errors reach the socket; log the trace, respond
        # with a compact 500 and let the connection close.
        traceback.print_exc()
        try:
            self.send_json(500, {"error": "Internal server error", "detail": str(exc)[:200]})
        except Exception:
            # Even the error response can fail (client already gone) — swallow.
            traceback.print_exc()

    # ---- HTTP method dispatchers (wrappers) -------------------------------

    def do_OPTIONS(self):
        try:
            self.send_response(200)
            self.send_header("Access-Control-Allow-Origin", "*")
            self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
            self.send_header("Access-Control-Allow-Headers", "Content-Type, Authorization")
            self.end_headers()
        except Exception as e:
            self._send_500(e)

    def do_GET(self):
        try:
            self._route_get()
        except Exception as e:
            self._send_500(e)

    def do_POST(self):
        try:
            self._route_post()
        except Exception as e:
            self._send_500(e)

    # ---- actual route bodies ---------------------------------------------

    def _route_get(self):
        parsed = urlparse(self.path)
        if parsed.path == "/health":
            self.send_json(200, {"status": "healthy", "timestamp": int(time.time()),
                                 "auth": "required" if AUTH_ENABLED else "disabled"})
            return

        if not self._check_auth():
            self._require_auth()
            return

        if parsed.path == "/api/location":
            params = parse_qs(parsed.query)
            code = params.get("code", [None])[0]
            if not code:
                self.send_json(400, {"error": "Missing 'code' parameter"})
                return

            now = int(time.time())
            row = db_conn.execute(
                "SELECT code, name, lat, lon, speed, bearing, updated_at "
                "FROM rider_locations WHERE code = ?", (code,)
            ).fetchone()

            if not row or (now - row[6] > LOCATION_TTL_SECONDS):
                self.send_json(404, {"error": "Rider location not found or expired", "code": code})
                return

            self.send_json(200, {
                "status": "ok",
                "code": row[0],
                "name": row[1],
                "lat": row[2],
                "lon": row[3],
                "speed": row[4],
                "bearing": row[5],
                "updated_at": row[6],
                "age_seconds": now - row[6],
            })
            return

        self.send_json(404, {"error": "Endpoint not found"})

    def _route_post(self):
        parsed = urlparse(self.path)

        if not self._check_auth():
            self._require_auth()
            return

        length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(length).decode("utf-8") if length > 0 else ""
        try:
            payload = json.loads(body) if body else {}
        except json.JSONDecodeError:
            self.send_json(400, {"error": "Invalid JSON"})
            return

        if parsed.path == "/api/location":
            code = payload.get("code")
            lat = payload.get("lat")
            lon = payload.get("lon")
            if not code or lat is None or lon is None:
                self.send_json(400, {"error": "'code', 'lat', 'lon' required"})
                return

            try:
                lat_f = float(lat)
                lon_f = float(lon)
                speed_f = float(payload.get("speed", 0))
                bearing_f = float(payload.get("bearing", 0))
            except (TypeError, ValueError):
                self.send_json(400, {"error": "'lat','lon','speed','bearing' must be numeric"})
                return

            now = int(time.time())
            name = str(payload.get("name", ""))[:64]
            with db_conn:
                db_conn.execute("""
                    INSERT INTO rider_locations (code, name, lat, lon, speed, bearing, updated_at)
                    VALUES (?, ?, ?, ?, ?, ?, ?)
                    ON CONFLICT(code) DO UPDATE SET
                        name=excluded.name,
                        lat=excluded.lat, lon=excluded.lon,
                        speed=excluded.speed, bearing=excluded.bearing,
                        updated_at=excluded.updated_at
                """, (code, name, lat_f, lon_f, speed_f, bearing_f, now))

            self.send_json(200, {"status": "success", "code": code, "updated_at": now})
            return

        if parsed.path == "/api/locations/batch":
            codes = payload.get("codes", [])
            if not isinstance(codes, list) or not codes:
                self.send_json(400, {"error": "'codes' list required"})
                return
            # Coerce to str, cap size to avoid a runaway query.
            codes = [str(c) for c in codes[:500]]

            now = int(time.time())
            placeholders = ",".join(["?"] * len(codes))
            rows = db_conn.execute(
                f"SELECT code, name, lat, lon, speed, bearing, updated_at "
                f"FROM rider_locations WHERE code IN ({placeholders})",
                codes,
            ).fetchall()

            results = {
                r[0]: {"name": r[1], "lat": r[2], "lon": r[3], "speed": r[4],
                       "bearing": r[5], "updated_at": r[6]}
                for r in rows if now - r[6] <= LOCATION_TTL_SECONDS
            }

            self.send_json(200, {"status": "ok", "riders": results})
            return

        self.send_json(404, {"error": "Endpoint not found"})

    # ---- logging ---------------------------------------------------------

    def log_message(self, format, *args):  # noqa: A002 - stdlib signature
        print(f"[{time.strftime('%H:%M:%S')}] {self.address_string()} - {format % args}",
              file=sys.stderr, flush=True)

    def log_error(self, format, *args):
        # BaseHTTPServer routes handler exceptions through here; keep them,
        # but they should be rare because handlers are wrapped in try/except.
        print(f"[{time.strftime('%H:%M:%S')}] ERROR {self.address_string()} - {format % args}",
              file=sys.stderr, flush=True)


def _handle_signal(signum, _frame):
    global _shutdown_requested
    _shutdown_requested = True
    print(f"\nSignal {signum} received — shutting down.", file=sys.stderr, flush=True)


def _serve_once():
    """Bring the server up and serve until it stops. Raises on failure."""
    server = ThreadingHTTPServer((HOST, PORT), RiderRequestHandler)
    server.daemon_threads = True  # don't wait on in-flight requests at shutdown
    try:
        print(f"Rider Tracking Server listening on {HOST}:{PORT}", flush=True)
        print(f"Auth: {'ENABLED (Basic)' if AUTH_ENABLED else 'DISABLED — set RIDER_USER + RIDER_PASS to enable'}",
              flush=True)
        server.serve_forever(poll_interval=0.5)
    finally:
        server.server_close()


def run_supervised():
    """Restart the server on unexpected failure with exponential backoff."""
    signal.signal(signal.SIGTERM, _handle_signal)
    signal.signal(signal.SIGINT, _handle_signal)

    backoff = RESTART_BACKOFF_START
    while not _shutdown_requested:
        try:
            _serve_once()
            # Clean exit (serve_forever returned) — probably a shutdown signal.
            break
        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"[{time.strftime('%H:%M:%S')}] Server crashed: {e!r}", file=sys.stderr, flush=True)
            traceback.print_exc()
            if _shutdown_requested:
                break
            print(f"Restarting in {backoff:.1f}s...", file=sys.stderr, flush=True)
            # Sleep in short chunks so a signal can interrupt the wait.
            slept = 0.0
            while slept < backoff and not _shutdown_requested:
                time.sleep(0.25)
                slept += 0.25
            backoff = min(backoff * 2.0, RESTART_BACKOFF_MAX)
        else:
            backoff = RESTART_BACKOFF_START

    print("Server stopped.", file=sys.stderr, flush=True)


if __name__ == "__main__":
    run_supervised()
