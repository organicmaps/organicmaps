#!/usr/bin/env python3
r"""Reproduce the original 80-second Android Drape queue stress test (Python 3.10+).

Example:
  python3 tools/python/drape_queue_stress.py --apk OrganicMaps-release.apk \
      --sdk /Users/vng/Library/Android/sdk --output /tmp/drape-stress-1

For tile cancellation diagnostics, add --random-delays. This keeps the same 500
jumps per phase but waits 50–500 ms between jumps (uniform integers, seed 0).
The saved scenario.json records every delay. This longer workload reports capture
validity and metrics without comparing against the original stress reference.

For stress across Hong Kong, add --distributed-points. This uses the same targets
as --wait-for-ready below, with the original 50 ms delay and no readiness barrier.
It can also be combined with --random-delays. The original reference gates do not
apply to distributed targets.

For loading throughput, use --wait-for-ready instead. Each jump waits for its
tiles and a subsequent RenderScene/Present before issuing the next jump. Reports
completed viewports/second and request-to-ready-frame latency, separately from
SurfaceFlinger presentation FPS (which also counts incomplete/unchanged frames).
Both phases jump to the same 500 distinct points spread across Hong Kong: a
32x24 grid filtered by data/borders/Hong Kong.poly, evenly sampled and shuffled
with seed 0. Pan stays at zoom 15; zoom changes cycle through 15/10/17. The border
includes territorial water, so some views contain little geometry. This reduces
repeated-location cache bias; it does not make OS or map-data caches cold.
This removes cancellation pressure, so it is not comparable to the stress reference.
An extra initial 5-second hold lets Android finish constructing the viewport.
Readiness is an engine barrier, not pixel verification. Each view has a 30-second
timeout; the whole capture is limited to 30 minutes.
Keep the host awake while capturing; on macOS, prefix the command with caffeinate -di.

--sdk points to the SDK root containing platform-tools/, emulator/ and build-tools/.
Without it, the runner uses ANDROID_SDK_ROOT, ANDROID_HOME, or the standard
macOS/Linux SDK location.

Build from android/: ./gradlew :app:assembleGoogleRelease -PdrapeBenchmark=ON
Add the ABI option for your emulator, e.g. -Parm64.
This enables DRAPE_QUEUE_TRACE and DRAPE_SCENARIO; retain native Release flags.
Debug signing is fine; the app
manifest must be non-debuggable. APK inspection cannot prove native optimizer
flags, so retain the Gradle/CMake build log. Stop builds before measuring.

Requires Android SDK adb, emulator and aapt, and a rootable emulator. The runner
reads the APK's map version from assets/countries.json and copies the repository's
./data/<version>/Hong Kong.mwm into the emulator after installing the APK.
It fails before launching or resetting an emulator if that host map is missing
or empty. No map download or preinstalled emulator map is required.

The default launch uses the Nexus_6 AVD's configuration in a private read-only,
no-snapshot session, removed on exit. Select another AVD with --avd. Using an
existing emulator requires BOTH --serial and --reset-benchmark-app; that permits
clearing app.organicmaps and changing emulator settings. Physical devices are
refused. Root access is required for Release app fixture setup and /proc sampling.

Reference: first completed stress run of vng-drape-queue 5d79ec7947, 2026-09-30,
Release on a host-accelerated Nexus_6 / Apple M1 Pro. The commit identifies the
archived measurement; it is not a required checkout for the APK being tested.
This reference used both batching (up to 64 shapes per message) and cancellation.
Gates are intentionally loose regression bounds: peak RSS <= 1.5x reference, frontend/backend message
peaks <= 3x reference. CPU, FPS and stalls are descriptive, never pass/fail gates.
They vary with the host and are not physical Nexus 6 performance predictions.

Exit 0: complete, valid capture within memory/queue limits; exit 1: setup/error;
exit 2: invalid/incomplete capture or regression. Raw logs are always preserved.
With --random-delays, --wait-for-ready or --distributed-points, exit 0 checks capture validity only,
without regression gates.
The output directory must not exist. SurfaceFlinger actual native presents are
used, not Java gfxinfo. Queue peaks end at the last sample, not necessarily the
scenario end; stale last sizes do not prove that a queue remained undrained.
"""
import argparse
import os
import shutil
import signal
import socket
import sys
import uuid
import zipfile
import calendar
import datetime as dt
import json
import math
import random
import re
import shlex
import statistics
import subprocess
import threading
import time
from pathlib import Path

LOG_TIME = re.compile(r"^(\d{2}-\d{2})\s+(\d{2}:\d{2}:\d{2}\.\d+)")
MAX_TIME = (1 << 63) - 1
PACKAGE = "app.organicmaps"
MAP_NAME = "Hong Kong.mwm"
# Names written by workload() into graphics_benchmark.json and echoed by the scenario runner.
PHASES = ("same-zoom-pan", "zoom-changes")
SETTINGS = """StoragePath=/data/data/app.organicmaps/files/
LastLocationStateMode=NotFollow
TtsEnabled=false
Units=Metric
Allow3d=true
Buildings3d=true
PreferredGraphicsAPI=OpenGLES3
MapStyleKeyV1=MapStyleDefaultLight
UiThemeSettings=default
KeepScreenOn=true
"""
REFERENCE = {
    "commit": "5d79ec7947", "run": "stress-after-1", "date": "2026-09-30",
    "overall": {"duration_s": 83.00500011444092, "fps": 18.45671944928373,
                "interval_p95_ms": 149.99999399999808, "interval_max_ms": 783.3333020000026,
                "gaps_over_250ms": 29, "app_cpu_percent": 127.48304287561399,
                "rss_mean_mib": 264.50447100903614, "rss_peak_mib": 363.65625,
                "frontend_queue_peak": 47, "backend_queue_peak": 574},
    "motion": {"duration_s": 52.955000162124634, "fps": 12.784439579403784,
               "interval_p95_ms": 199.99999200000218, "interval_max_ms": 749.9999699999976,
               "gaps_over_250ms": 25, "app_cpu_percent": 169.15655849304358,
               "rss_mean_mib": 260.69590773809523, "rss_peak_mib": 363.4765625},
}
LIMITS = {"rss_peak_mib": REFERENCE["overall"]["rss_peak_mib"] * 1.5,
          "frontend_queue_peak": REFERENCE["overall"]["frontend_queue_peak"] * 3,
          "backend_queue_peak": REFERENCE["overall"]["backend_queue_peak"] * 3}


def hong_kong_targets():
    border = Path(__file__).resolve().parents[2] / "data/borders/Hong Kong.poly"
    lines = border.read_text().splitlines()
    if lines[:2] != ["Hong Kong", "1"] or lines[-2:] != ["END", "END"]:
        raise RuntimeError(f"Expected one exterior ring in {border}")
    polygon = [tuple(map(float, line.split())) for line in lines[2:-2]]
    west, east = min(p[0] for p in polygon), max(p[0] for p in polygon)
    south, north = min(p[1] for p in polygon), max(p[1] for p in polygon)

    def inside(lon, lat):
        contained = False
        x1, y1 = polygon[-1]
        for x2, y2 in polygon:
            if (y1 > lat) != (y2 > lat) and lon < x1 + (lat - y1) * (x2 - x1) / (y2 - y1):
                contained = not contained
            x1, y1 = x2, y2
        return contained

    candidates = []
    for row in range(24):
        lat = south + (row + 0.5) * (north - south) / 24
        for column in range(32):
            lon = west + (column + 0.5) * (east - west) / 32
            if inside(lon, lat):
                candidates.append((lat, lon))
    if len(candidates) < 500:
        raise RuntimeError(f"Hong Kong border provides only {len(candidates)} benchmark targets")
    targets = [candidates[i * len(candidates) // 500] for i in range(500)]
    random.Random(0).shuffle(targets)
    return targets


def workload(random_delays=False, wait_for_ready=False, distributed_points=False):
    def center(lon, zoom, lat=22.302):
        step = {"actionType": "centerViewport", "center": {"lat": lat, "lon": lon},
                "zoomLevel": zoom, "animated": False}
        if wait_for_ready:
            step["waitForReady"] = True
        return step

    def wait(milliseconds):
        return {"actionType": "waitForTime", "timeMs": milliseconds}

    rng = random.Random(0)
    targets = hong_kong_targets() if wait_for_ready or distributed_points else [
        (22.302, 114.17 + (0, 1, 0, -1)[i % 4] * 0.02) for i in range(500)]
    scenarios = []
    for name in PHASES:
        steps = [center(114.17, 15), wait(5000)]
        if wait_for_ready and name == PHASES[0]:
            steps.insert(0, wait(5000))
        for i, (lat, lon) in enumerate(targets):
            zoom = 15 if name == PHASES[0] else (15, 10, 17)[i % 3]
            steps.append(center(lon, zoom, lat))
            if not wait_for_ready:
                steps.append(wait(rng.randint(50, 500) if random_delays else 50))
        steps += [center(114.17, 15), wait(10000)]
        scenarios.append({"name": name, "steps": steps})
    return (json.dumps({"scenarios": scenarios}, indent=2) + "\n").encode()


def scenario_timings(scenarios):
    timings = []
    for scenario in scenarios:
        steps = scenario["steps"]
        centers = [index for index, step in enumerate(steps) if step["actionType"] == "centerViewport"]
        timings.append({"duration_s": sum(step.get("timeMs", 0) for step in steps) / 1000,
                        "warmup_s": sum(step.get("timeMs", 0) for step in steps[:centers[1]]) / 1000,
                        "drain_s": sum(step.get("timeMs", 0) for step in steps[centers[-1]:]) / 1000})
    return timings


def adb(args, *command, timeout=15):
    return subprocess.run([args.adb, "-s", args.serial, *command], stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True, timeout=timeout, check=True).stdout.replace("\r", "")


def elapsed(text):
    units = {"d": 86400, "h": 3600, "m": 60, "s": 1, "ms": 0.001}
    return sum(float(n) * units[u] for n, u in re.findall(r"(\d+)(ms|d|h|m|s)", text))


def clock_anchor(text):
    match = re.search(r"nowRTC=(\d+)=(\d{4}-\d{2}-\d{2} \d{2}:\d{2}:\d{2}) nowELAPSED=([^\n]+)", text)
    if not match:
        raise RuntimeError("Cannot align clocks: dumpsys alarm lacks nowRTC/nowELAPSED")
    rtc, local, elapsed_text = match.groups()
    local_epoch = calendar.timegm(dt.datetime.strptime(local, "%Y-%m-%d %H:%M:%S").timetuple()) + int(rtc) % 1000 / 1000
    uptime = elapsed(elapsed_text)
    return {"rtc_epoch_s": int(rtc) / 1000, "local_datetime": local, "elapsed_s": uptime,
            "local_epoch_minus_elapsed_s": local_epoch - uptime, "year": int(local[:4])}


def log_uptime(line, anchor):
    match = LOG_TIME.match(line)
    if not match:
        return None
    stamp = dt.datetime.strptime(f"{anchor['year']}-{match[1]} {match[2]}", "%Y-%m-%d %H:%M:%S.%f")
    return calendar.timegm(stamp.timetuple()) + stamp.microsecond / 1e6 - anchor["local_epoch_minus_elapsed_s"]


def process_stat(text):
    # comm can contain spaces or parentheses; fields after the final ')' begin at state (field 3).
    fields = text.rsplit(")", 1)[1].split()
    return {"ticks": int(fields[11]) + int(fields[12]), "start_ticks": int(fields[19])}


def find_pid(ps, name):
    for line in ps.splitlines():
        fields = line.split()
        if len(fields) > 2 and fields[-1] == name and fields[1].isdigit():
            return int(fields[1])
    return None


def blocks(text):
    result = {}
    for name, body in re.findall(r"^__([A-Z_]+)__\n(.*?)(?=^__[A-Z_]+__\n|\Z)", text, re.M | re.S):
        result[name] = body.strip()
    return result


def percentile(values, p):
    if not values:
        return None
    values = sorted(values)
    pos = (len(values) - 1) * p / 100
    lower = math.floor(pos)
    return values[lower] + (values[math.ceil(pos)] - values[lower]) * (pos - lower)


class QueueDecoder:
    """Accept original snapshots and loss-detecting multipart Android snapshots."""
    def __init__(self):
        self.pending = {}
        self.last_sample = {}

    def feed(self, data, stamp):
        fields = ("sample", "part", "parts")
        if not any(key in data for key in fields):
            return {"uptime_s": stamp, "data": data}
        if not all(type(data.get(key)) is int for key in fields):
            raise ValueError("Incomplete/noninteger queue chunk metadata")
        renderer, sample, part, parts = data["renderer"], data["sample"], data["part"], data["parts"]
        if sample < 1 or parts < 1 or not 0 <= part < parts:
            raise ValueError("Invalid queue sample/part bounds")
        key = (renderer, sample)
        if sample != self.last_sample.get(renderer, 0) + 1:
            raise ValueError("Missing/duplicate/out-of-order queue sample")
        header = (data["size"], data["peak"], parts)
        entry = self.pending.setdefault(key, {"header": header, "parts": {}, "uptime_s": stamp})
        if entry["header"] != header or part in entry["parts"]:
            raise ValueError("Conflicting/duplicate queue chunk")
        entry["parts"][part] = data["types"]
        if len(entry["parts"]) != parts:
            return None
        types = {}
        for index in range(parts):
            chunk = entry["parts"][index]
            if types.keys() & chunk.keys():
                raise ValueError("Queue type duplicated across chunks")
            types.update(chunk)
        del self.pending[key]
        self.last_sample[renderer] = sample
        return {"uptime_s": entry["uptime_s"], "data": {
            "renderer": renderer, "size": header[0], "peak": header[1], "sample": sample, "types": types}}


class ProcessPresence:
    """Require repeated ps-confirmed absence; missing /proc alone triggers discovery."""
    def __init__(self):
        self.ever_seen = False
        self.last_seen_uptime_s = None
        self.absent_since = None
        self.absent_polls = 0
        self.evidence = None

    def update(self, uptime, proc_present, ps_checked, ps_pid):
        if proc_present or (ps_checked and ps_pid is not None):
            self.ever_seen = True
            self.last_seen_uptime_s = uptime
            self.absent_since = None
            self.absent_polls = 0
            return
        if not self.ever_seen or not ps_checked:
            return
        if self.absent_since is None:
            self.absent_since = uptime
        self.absent_polls += 1
        if self.evidence is None and self.absent_polls >= 3 and uptime - self.absent_since >= 3.0:
            self.evidence = {"last_seen_uptime_s": self.last_seen_uptime_s,
                             "first_absent_uptime_s": self.absent_since, "confirmed_uptime_s": uptime,
                             "consecutive_ps_absent_polls": self.absent_polls,
                             "reason": "Previously observed app absent from both /proc stat and repeated ps snapshots; "
                                       "cause unknown"}


def capture(args):
    args.output.mkdir(parents=True, exist_ok=True)
    if (args.output / "metadata.json").exists():
        raise SystemExit(f"Refusing to overwrite existing capture: {args.output}")
    anchor = clock_anchor(adb(args, "shell", "dumpsys alarm"))
    ps = adb(args, "shell", "ps")
    sf_pid = find_pid(ps, "/system/bin/surfaceflinger") or find_pid(ps, "surfaceflinger")
    metadata = {"package": args.package, "serial": args.serial, "clock_anchor": anchor,
                "sample_interval_s": args.interval, "clk_tck": 100, "surfaceflinger_pid": sf_pid,
                "method": "SurfaceFlinger --latency second column actual-present nanoseconds; deduplicated",
                "cpu_convention": "100% = one core; Android Linux USER_HZ=100",
                "clock_note": "AlarmManager realtime/elapsed millisecond anchor; screen must remain awake (no suspend)",
                "host_started": dt.datetime.now(dt.timezone.utc).isoformat(), "warnings": []}
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    done = threading.Event()
    finished = threading.Event()
    process_deaths = []
    queue_records = []
    markers = []
    malformed = []
    decoder = QueueDecoder()
    log_proc = subprocess.Popen([args.adb, "-s", args.serial, "logcat", "-v", "threadtime"],
                                stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)

    def read_log():
        with (args.output / "logcat.txt").open("w") as raw, (args.output / "queues.jsonl").open("w") as queues:
            for line in log_proc.stdout:
                raw.write(line)
                raw.flush()
                stamp = log_uptime(line, anchor)
                marker = re.search(r"Drape scenario (started|finished|failed):\s*(\S+)", line)
                if marker:
                    markers.append({"kind": marker[1], "name": marker[2], "uptime_s": stamp})
                    if marker[1] == "failed":
                        done.set()
                if "Drape benchmark finished" in line:
                    markers.append({"kind": "benchmark_finished", "uptime_s": stamp})
                    finished.set()
                    done.set()
                if re.search(r"Process " + re.escape(args.package) + r" \(pid \d+\) has died", line):
                    process_deaths.append({"uptime_s": stamp, "line": line.strip()})
                    if not finished.is_set():
                        done.set()
                if "DrapeQueue " in line:
                    try:
                        data = json.loads(line.split("DrapeQueue ", 1)[1])
                        record = decoder.feed(data, stamp)
                        if record is not None:
                            queue_records.append(record)
                            queues.write(json.dumps(record) + "\n")
                            queues.flush()
                    except (ValueError, KeyError, TypeError) as exc:
                        malformed.append({"uptime_s": stamp, "line_bytes": len(line.encode()), "error": str(exc)})

    reader = threading.Thread(target=read_log, daemon=True)
    reader.start()
    start = time.monotonic()
    wall_start = time.time()
    deadline = start + args.duration_max
    layer = None
    app_pid = None
    sample_count = 0
    frame_count = 0
    seen = set()
    collect_errors = []
    presence = ProcessPresence()
    unexpected_logcat_exit = None
    print(f"Capturing {args.output}", flush=True)
    try:
        (args.output / "launch.txt").write_text(adb(args, "shell", "am start -n " + args.package + "/.SplashActivity"))
        with (args.output / "samples.jsonl").open("w") as samples, (args.output / "frames.jsonl").open("w") as frames:
            next_sample = time.monotonic()
            stop_at = None
            while time.monotonic() < deadline:
                now = time.monotonic()
                if unexpected_logcat_exit is None and log_proc.poll() is not None:
                    unexpected_logcat_exit = {"exit_code": log_proc.returncode, "host_elapsed_s": now - start}
                    collect_errors.append(
                        f"logcat exited unexpectedly during collection (code {log_proc.returncode}); "
                        "marker and queue logs may be incomplete")
                    done.set()
                if done.is_set() and stop_at is None:
                    stop_at = now + 1.0  # Capture SurfaceFlinger's last presents and final log records.
                if stop_at is not None and now > stop_at:
                    break
                discover = app_pid is None or layer is None or sample_count % 20 == 0
                parts = ["echo __UPTIME__", "cat /proc/uptime"]
                if discover:
                    parts += ["echo __PS__", "ps", "echo __LAYERS__", "dumpsys SurfaceFlinger --list"]
                if app_pid:
                    parts += ["echo __APP_STAT__", f"cat /proc/{app_pid}/stat",
                              "echo __APP_STATUS__", f"cat /proc/{app_pid}/status"]
                if sf_pid:
                    parts += ["echo __SF_STAT__", f"cat /proc/{sf_pid}/stat"]
                if layer:
                    parts += ["echo __LATENCY__", "dumpsys SurfaceFlinger --latency " + shlex.quote(layer)]
                parts += ["echo __UPTIME_END__", "cat /proc/uptime"]
                try:
                    host_begin = time.monotonic()
                    output = blocks(adb(args, "shell", "; ".join(parts)))
                    host_end = time.monotonic()
                    up_start = float(output["UPTIME"].split()[0])
                    up_end = float(output["UPTIME_END"].split()[0])
                    sample = {"uptime_s": up_start, "uptime_end_s": up_end,
                              "poll_host_ms": (host_end - host_begin) * 1000, "app_pid": app_pid, "layer": layer}
                    if "PS" in output:
                        app_pid = find_pid(output["PS"], args.package)
                        sample["app_present_in_ps"] = app_pid is not None
                        candidates = [item for item in output["LAYERS"].splitlines() if "SurfaceView" in item]
                        exact = [item for item in candidates if args.package in item]
                        options = exact or candidates
                        if len(options) == 1:
                            layer = options[0]
                        elif len(options) != 1:
                            layer = None
                            sample["surface_candidates"] = candidates
                    for label in ("APP", "SF"):
                        try:
                            sample[label.lower()] = process_stat(output[label + "_STAT"])
                        except (KeyError, ValueError, IndexError):
                            pass
                    presence.update(up_start, "app" in sample, "PS" in output, app_pid)
                    if presence.evidence and not finished.is_set():
                        done.set()
                    if "app" not in sample and presence.ever_seen and "PS" not in output:
                        app_pid = None  # Force ps next poll after losing a previously readable /proc stat.
                    match = re.search(r"^VmRSS:\s+(\d+)\s+kB", output.get("APP_STATUS", ""), re.M)
                    if match:
                        sample["rss_kib"] = int(match[1])
                    latency = output.get("LATENCY", "").splitlines()
                    if latency and latency[0].isdigit():
                        sample["refresh_period_ns"] = int(latency[0])
                    batch = []
                    for line in latency[1:]:
                        fields = line.split()
                        if len(fields) != 3 or not all(item.isdigit() for item in fields):
                            continue
                        desired, actual, ready = map(int, fields)
                        if actual <= 0 or actual >= MAX_TIME or actual in seen:
                            continue
                        seen.add(actual)
                        frame = {"present_ns": actual, "desired_ns": desired, "ready_ns": ready,
                                 "observed_uptime_s": up_end, "layer": sample["layer"]}
                        frames.write(json.dumps(frame) + "\n")
                        frame_count += 1
                        batch.append(actual)
                    sample["new_frames"] = len(batch)
                    samples.write(json.dumps(sample) + "\n")
                    samples.flush()
                    frames.flush()
                    sample_count += 1
                except (subprocess.SubprocessError, KeyError, ValueError) as exc:
                    collect_errors.append(str(exc))
                next_sample += args.interval
                time.sleep(max(0, next_sample - time.monotonic()))
    finally:
        log_proc.terminate()
        try:
            log_proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            log_proc.kill()
        reader.join(timeout=5)
    scenario_failed = any(marker["kind"] == "failed" for marker in markers)
    metadata.update({"finished_marker": finished.is_set(), "process_deaths": process_deaths,
                     "capture_status": ("scenario_failed" if scenario_failed else
                                        "completed" if finished.is_set() else "process_died" if process_deaths
                                        else "app_disappeared" if presence.evidence else "timeout"),
                     "app_absence_evidence": presence.evidence, "unexpected_logcat_exit": unexpected_logcat_exit,
                     "markers": markers, "samples": sample_count,
                     "frames": frame_count, "malformed_queue_logs": malformed, "collection_errors": collect_errors,
                     "host_capture_duration_s": time.monotonic() - start,
                     "host_wall_duration_s": time.time() - wall_start})
    metadata["incomplete_queue_chunks"] = [list(key) for key in decoder.pending]
    try:
        metadata["clock_anchor_end"] = clock_anchor(adb(args, "shell", "dumpsys alarm"))
        drift = metadata["clock_anchor_end"]["local_epoch_minus_elapsed_s"] - anchor["local_epoch_minus_elapsed_s"]
        metadata["clock_drift_ms"] = drift * 1000
        if abs(drift) > 0.020:
            metadata["warnings"].append("Device wall-clock alignment changed by >20 ms during capture")
    except Exception as exc:
        metadata["warnings"].append(f"Ending clock anchor failed: {exc}")
    if malformed:
        metadata["warnings"].append("Malformed queue JSON: Android log payload truncation is possible; "
                                    "queue totals may be incomplete")
    if not finished.is_set():
        metadata["warnings"].append("Scenario reported failure; incomplete run" if scenario_failed else
                                    "App process died before benchmark completion; incomplete run" if process_deaths
                                    else "Previously observed app disappeared for at least 3 seconds; "
                                         "cause unknown; incomplete run" if presence.evidence
                                    else "Timed out without Drape benchmark finished; incomplete run")
    if unexpected_logcat_exit:
        metadata["warnings"].append("logcat exited unexpectedly; markers and queue traces may be incomplete")
    if not seen:
        metadata["warnings"].append("No native SurfaceView present timestamps captured")
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    print(f"DONE {args.output}: {sample_count} samples, {frame_count} frames, "
          f"finished={finished.is_set()}, malformed_queues={len(malformed)}", flush=True)
    return 0 if finished.is_set() else 2


def read_jsonl(path):
    return [json.loads(line) for line in path.read_text().splitlines() if line]


def metrics(samples, frames, windows):
    duration = sum(end - start for start, end in windows)
    intervals, count = [], 0
    for start, end in windows:
        presents = sorted({f["present_ns"] / 1e9 for f in frames if start <= f["present_ns"] / 1e9 <= end})
        count += len(presents)
        intervals += [(b - a) * 1000 for a, b in zip(presents, presents[1:])]
    result = {"duration_s": duration, "presented_frames": count, "fps": count / duration,
              "interval_p95_ms": percentile(intervals, 95), "interval_max_ms": max(intervals, default=None),
              "gaps_over_250ms": sum(value > 250 for value in intervals)}
    for kind in ("app", "sf"):
        ticks = coverage = 0
        for a, b in zip(samples, samples[1:]):
            if kind not in a or kind not in b or a[kind]["start_ticks"] != b[kind]["start_ticks"]:
                continue
            span = b["uptime_s"] - a["uptime_s"]
            overlap = sum(max(0, min(end, b["uptime_s"]) - max(start, a["uptime_s"])) for start, end in windows)
            delta = b[kind]["ticks"] - a[kind]["ticks"]
            if span > 0 and overlap > 0 and delta >= 0:
                ticks += delta * overlap / span
                coverage += overlap
        result[kind + "_cpu_percent"] = ticks / coverage if coverage else None  # USER_HZ=100, percent=100.
        result[kind + "_cpu_coverage_fraction"] = coverage / duration
    rss = [sample["rss_kib"] / 1024 for sample in samples if "rss_kib" in sample and
           any(start <= sample["uptime_s"] <= end for start, end in windows)]
    result.update(rss_mean_mib=statistics.mean(rss) if rss else None, rss_peak_mib=max(rss, default=None))
    return result


def ready_viewports(output, scenarios, windows, anchor):
    records = []
    for line in (output / "logcat.txt").read_text().splitlines():
        if "DrapeViewport " in line:
            data = json.loads(line.split("DrapeViewport ", 1)[1])
            records.append({"uptime_s": log_uptime(line, anchor), **data})
    expected = [(scenario["name"], index)
                for scenario in scenarios
                for index, step in enumerate(s for s in scenario["steps"] if s["actionType"] == "centerViewport")
                if step.get("waitForReady")]
    if [(record.get("scenario"), record.get("index")) for record in records] != expected:
        raise ValueError("Missing, duplicated or out-of-order DrapeViewport acknowledgements")
    motion_windows, phases, all_latencies = [], {}, []
    for scenario, (start, end) in zip(scenarios, windows):
        phase = [record for record in records if record["scenario"] == scenario["name"]]
        previous = start
        for record in phase:
            elapsed_ms, stamp = record.get("elapsed_ms"), record["uptime_s"]
            if (record.get("ready") is not True or type(elapsed_ms) not in (int, float) or
                    not math.isfinite(elapsed_ms) or not 0 < elapsed_ms <= 30000 or
                    stamp is None or not previous <= stamp <= end or stamp - elapsed_ms / 1000 < previous - 0.002):
                raise ValueError("Failed or inconsistent DrapeViewport acknowledgement")
            previous = stamp
        # Initial and final centers are warmup/drain, exactly as in the timed workload.
        motion = phase[1:-1]
        if not motion:
            raise ValueError("No synchronized motion viewports")
        motion_windows.append((motion[0]["uptime_s"] - motion[0]["elapsed_ms"] / 1000,
                               motion[-1]["uptime_s"]))
        latencies = [record["elapsed_ms"] for record in motion]
        all_latencies += latencies
        phases[scenario["name"]] = viewport_metrics(latencies, [motion_windows[-1]])
    result = viewport_metrics(all_latencies, motion_windows)
    result["phases"] = phases
    return result, motion_windows


def viewport_metrics(latencies, windows):
    duration = sum(end - start for start, end in windows)
    return {"completed_viewports": len(latencies), "duration_s": duration,
            "viewports_per_s": len(latencies) / duration,
            "latency_median_ms": statistics.median(latencies),
            "latency_p95_ms": percentile(latencies, 95), "latency_max_ms": max(latencies)}


def evaluate(output):
    scenarios = json.loads((output / "scenario.json").read_text())["scenarios"]
    timings = scenario_timings(scenarios)
    comparable = scenarios == json.loads(workload())["scenarios"]
    synchronized = any(step.get("waitForReady") for scenario in scenarios for step in scenario["steps"])
    metadata = json.loads((output / "metadata.json").read_text())
    samples, frames, queues = (read_jsonl(output / (name + ".jsonl")) for name in ("samples", "frames", "queues"))
    errors = list(metadata.get("warnings", [])) + list(metadata.get("collection_errors", []))
    for key in ("process_deaths", "app_absence_evidence", "unexpected_logcat_exit",
                "malformed_queue_logs", "incomplete_queue_chunks"):
        if metadata.get(key):
            errors.append(f"Capture failure: {key}: {metadata[key]}")
    drift = metadata.get("clock_drift_ms")
    if drift is None or abs(drift) > 20:
        errors.append(f"Clock alignment missing or changed by more than 20 ms: {drift}")
    # Host monotonic and emulator clocks can both pause during laptop sleep.
    if ("host_wall_duration_s" in metadata and
            abs(metadata["host_wall_duration_s"] - metadata["host_capture_duration_s"]) > 1):
        errors.append("Host suspend or wall-clock change during capture (>1 second)")
    if len({sample["app"]["start_ticks"] for sample in samples if "app" in sample}) != 1:
        errors.append("App process was not observed continuously with one start identity")
    if any(sample.get("new_frames", 0) >= 127 for sample in samples[1:]):
        errors.append("Possible SurfaceFlinger 128-frame ring overrun")
    if any(len(sample.get("surface_candidates", [])) > 1 for sample in samples):
        errors.append("Ambiguous native SurfaceView layer")
    markers = metadata.get("markers", [])
    expected = [(kind, name) for name in PHASES for kind in ("started", "finished")] + [("benchmark_finished", None)]
    actual = [(marker["kind"], marker.get("name")) for marker in markers]
    windows = []
    if not metadata.get("finished_marker") or actual != expected:
        errors.append(f"Expected both complete phases followed by benchmark completion; got {actual}")
    elif any(marker.get("uptime_s") is None for marker in markers):
        errors.append("Scenario marker has no aligned timestamp")
    else:
        stamps = [marker["uptime_s"] for marker in markers]
        windows = [(stamps[0], stamps[1]), (stamps[2], stamps[3])]
        if stamps != sorted(stamps) or any(end - start < timing["duration_s"] - 0.5
                                           for (start, end), timing in zip(windows, timings)):
            errors.append("Out-of-order or unexpectedly short scenario phases")
            windows = []
    conservation = []
    for record in queues:
        try:
            data = record["data"]
            if data["renderer"] not in ("frontend", "backend") or record["uptime_s"] is None:
                raise ValueError("Invalid renderer or queue timestamp")
            if any(type(data[key]) is not int or data[key] < 0 for key in ("size", "peak")):
                raise ValueError("Invalid queue size/peak")
            for name, counts in data["types"].items():
                for key in ("size", "peak", "enqueued", "popped", "filtered", "rejected", "cleared"):
                    if type(counts[key]) is not int or counts[key] < 0:
                        raise ValueError(f"Invalid {name}.{key}")
                if counts["enqueued"] != sum(counts[k] for k in ("size", "popped", "filtered", "cleared")):
                    raise ValueError(f"Counter conservation failed for {name}")
            if data["size"] != sum(c["size"] for c in data["types"].values()) or data["peak"] < data["size"]:
                raise ValueError("Queue total/peak is inconsistent")
        except (KeyError, TypeError, ValueError) as exc:
            conservation.append(str(exc))
    errors.extend(conservation)
    observed, queue_endpoints, viewports = {}, {}, None
    if windows:
        start, end = windows[0][0], windows[-1][1]
        observed["overall"] = metrics(samples, frames, [(start, end)])
        motion_windows = [(a + timing["warmup_s"], b - timing["drain_s"])
                          for (a, b), timing in zip(windows, timings)]
        if synchronized:
            try:
                viewports, motion_windows = ready_viewports(output, scenarios, windows, metadata["clock_anchor"])
                for scenario, window in zip(scenarios, motion_windows):
                    phase = viewports["phases"][scenario["name"]]
                    phase["presentation_fps"] = metrics(samples, frames, [window])["fps"]
            except (OSError, ValueError, KeyError, TypeError) as exc:
                errors.append(f"Invalid synchronized capture: {exc}")
                motion_windows = []
        if motion_windows:
            observed["motion"] = metrics(samples, frames, motion_windows)
            if viewports:
                viewports["presentation_fps"] = observed["motion"]["fps"]
        for renderer in ("frontend", "backend"):
            eligible = [record for record in queues if record["data"].get("renderer") == renderer and
                        record.get("uptime_s") is not None and record["uptime_s"] <= end]
            if not eligible or any(not any(a <= q["uptime_s"] <= b for q in eligible) for a, b in windows):
                errors.append(f"Missing {renderer} queue samples in a phase; build with -PdrapeBenchmark=ON")
                continue
            # Ready acknowledgements prove backend progress even if a slow final render follows its last trace.
            # Timed stress needs fresh queue traces: its finish marker alone says nothing about backend progress.
            if not synchronized and any(not any(b - 1.5 <= q["uptime_s"] <= phase_end for q in eligible)
                                        for (_, b), (_, phase_end) in zip(motion_windows, windows)):
                errors.append(f"Stale {renderer} queue trace before the end of a phase's motion")
            observed["overall"][renderer + "_queue_peak"] = max(record["data"]["peak"] for record in eligible)
            queue_endpoints[renderer] = {"samples": len(eligible), "last_size": eligible[-1]["data"]["size"],
                                         "last_sample_age_at_end_s": end - eligible[-1]["uptime_s"]}
        if observed["overall"]["presented_frames"] < 2:
            errors.append("No usable native SurfaceView frames")
        if observed["overall"]["app_cpu_coverage_fraction"] < 0.95:
            errors.append("Process sampling covered less than 95% of the scenario")
        polls = [sample for sample in samples if start <= sample["uptime_s"] <= end]
        rss_stamps = [sample["uptime_s"] for sample in polls if "rss_kib" in sample]
        if not polls or len(rss_stamps) / len(polls) < 0.95:
            errors.append("RSS missing from more than 5% of in-window polls; sampled peak is unreliable")
        edges = [start, *rss_stamps, end]
        max_gap = max(2.0, 4 * metadata.get("sample_interval_s", 0.5))
        if any(b - a > max_gap for a, b in zip(edges, edges[1:])):
            errors.append("RSS polling has gaps; sampled peak is unreliable")
    gates = {}
    for metric, limit in (LIMITS.items() if comparable else ()):
        value = observed.get("overall", {}).get(metric)
        gates[metric] = {"value": value, "maximum": limit, "passed": value is not None and value <= limit}
    valid = bool(observed) and not errors
    result = {"reference": REFERENCE if comparable else None, "reference_comparable": comparable,
              "observed": observed, "queue_endpoints": queue_endpoints, "capture_valid": valid,
              "capture_errors": errors, "limits": gates,
              "passed": valid and all(gate["passed"] for gate in gates.values()) if comparable else None,
              "deltas_percent": {window: {key: (values[key] / reference - 1) * 100
                  for key, reference in REFERENCE[window].items() if values.get(key) is not None and reference}
                  for window, values in observed.items()} if comparable else {}}
    if synchronized:
        result["ready_viewports"] = viewports
    (output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    lines = (["PASS" if result["passed"] else "FAIL",
              "Original vng-drape-queue 5d79ec7947 stress reference vs this run"] if comparable else
             ["VALID DIAGNOSTIC CAPTURE" if valid else "INVALID DIAGNOSTIC CAPTURE",
              "Different workload; original-reference comparison and regression gates are disabled."])
    if viewports:
        lines += ["", "Synchronized viewport completion (warmup/drain excluded):",
                  f"{'phase':20s} {'views':>6s} {'views/s':>9s} {'present FPS':>12s} "
                  f"{'median ms':>11s} {'p95 ms':>10s} {'max ms':>10s}"]
        for name, values in [("all motion", viewports), *viewports["phases"].items()]:
            lines.append(f"{name:20s} {values['completed_viewports']:6d} {values['viewports_per_s']:9.3f} "
                         f"{values['presentation_fps']:12.3f} "
                         f"{values['latency_median_ms']:11.3f} {values['latency_p95_ms']:10.3f} "
                         f"{values['latency_max_ms']:10.3f}")
        lines += ["Presentation FPS below also counts incomplete/unchanged frames."]
    for window, values in observed.items():
        lines += ["", window, (f"{'metric':28s} {'reference':>12s} {'observed':>12s} {'delta':>10s}" if comparable else
                               f"{'metric':28s} {'observed':>12s}")]
        for key, reference in REFERENCE[window].items():
            value = values.get(key)
            if value is not None:
                lines.append(f"{key:28s} {reference:12.3f} {value:12.3f} "
                             f"{result['deltas_percent'][window][key]:+9.1f}%" if comparable else
                             f"{key:28s} {value:12.3f}")
    if comparable:
        lines += ["", "Hard gates (memory 1.5x, queue peaks 3x; CPU/FPS/stalls are report-only):"]
    lines += [f"{name}: {gate['value']} <= {gate['maximum']:.3f}: {'PASS' if gate['passed'] else 'FAIL'}"
              for name, gate in gates.items()]
    lines += ["ERROR: " + error for error in errors]
    lines += ["", "Capture validity does not check whether the map is visibly complete.",
              "Queue peaks cover startup through the last sample before completion; see result.json for sample ages."]
    if comparable:
        lines.append("Message counts are comparable only with the same batching policy as the reference.")
    report = "\n".join(lines) + "\n"
    (output / "result.txt").write_text(report)
    print(report, end="")
    return result


def run(command, timeout=30):
    return subprocess.run([str(part) for part in command], text=True, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, timeout=timeout, check=True).stdout


def tools_from_sdk(args):
    sdk = args.sdk.expanduser()
    for name, relative in (("adb", "platform-tools/adb"), ("emulator", "emulator/emulator")):
        value = getattr(args, name) or str(sdk / relative)
        setattr(args, name, shutil.which(value) or value)
    if not args.aapt:
        candidates = list((sdk / "build-tools").glob("*/aapt"))
        if candidates:
            args.aapt = str(max(candidates, key=lambda p: tuple(map(int, re.findall(r"\d+", p.parent.name)))))
    for name in ("adb", "emulator", "aapt"):
        value = getattr(args, name)
        if not value or not Path(value).is_file():
            raise RuntimeError(f"Cannot find {name}; supply --sdk or --{name}")


def verify_apk(args):
    manifest = run([args.aapt, "dump", "badging", args.apk])
    if not re.search(r"^package: name='app\.organicmaps' ", manifest, re.M):
        raise RuntimeError("APK must use the Release package app.organicmaps")
    if "application-debuggable" in manifest:
        raise RuntimeError("Debuggable APK refused: build Google Release")
    with zipfile.ZipFile(args.apk) as archive:
        args.map_version = str(int(json.loads(archive.read("assets/countries.json"))["v"]))
    return manifest


def free_emulator_port(args):
    attached = run([args.adb, "devices"])
    for port in range(5554, 5682, 2):
        if f"emulator-{port}" in attached:
            continue
        sockets = []
        try:
            for number in (port, port + 1):
                sock = socket.socket()
                sockets.append(sock)
                sock.bind(("127.0.0.1", number))
            return port
        except OSError:
            pass
        finally:
            for sock in sockets:
                sock.close()
    raise RuntimeError("No unused emulator console/ADB port pair found")


def wait_for_boot(args, emulator):
    deadline = time.monotonic() + 120
    while time.monotonic() < deadline:
        if emulator is not None and emulator.poll() is not None:
            raise RuntimeError("Owned emulator exited; inspect emulator.log")
        try:
            if adb(args, "shell", "getprop sys.boot_completed", timeout=5).strip() == "1":
                return
        except subprocess.SubprocessError:
            pass
        time.sleep(1)
    raise RuntimeError("Emulator failed to boot within 120 seconds")


def shell_uid(output):
    text = (output or "").strip()
    match = re.search(r"(?:^|\s)uid=(\d+)\b", text)
    return int(match[1]) if match else int(text) if text.isdecimal() else None


def require_root(args):
    # API21 toolbox id does not support -u; plain id works across Android versions.
    with (args.output / "root-attempts.txt").open("w") as log:
        def attempt(*command):
            try:
                output = adb(args, *command, timeout=5)
            except subprocess.SubprocessError as exc:
                output = getattr(exc, "stdout", None) or str(exc)
                log.write(f"adb {shlex.join(command)}: ERROR\n{output}\n")
                log.flush()
                return None
            log.write(f"adb {shlex.join(command)}\n{output}\n")
            log.flush()
            return output

        if shell_uid(attempt("shell", "id")) == 0:
            return
        response = attempt("root")
        if response and "cannot run as root" in response.lower():
            raise RuntimeError("adbd cannot run as root on this image; see root-attempts.txt")
        # wait-for-device can return the old transport before adbd restarts. Verify
        # the actual shell UID, tolerating disconnection and the old non-root daemon.
        deadline = time.monotonic() + 30
        while time.monotonic() < deadline:
            if shell_uid(attempt("shell", "id")) == 0:
                return
            time.sleep(1)
    raise RuntimeError("Emulator shell did not become root within 30 seconds; use a rootable emulator "
                       "image and inspect root-attempts.txt")


def get_map_path(version):
    path = Path(__file__).resolve().parents[2] / "data" / version / MAP_NAME
    if not path.is_file() or path.stat().st_size == 0:
        raise RuntimeError(f"Required host map is missing or empty: {path}")
    return path


def prepare(args, nonce):
    if adb(args, "shell", "getprop ro.kernel.qemu").strip() != "1":
        raise RuntimeError("Refusing to modify a non-emulator device")
    if nonce and adb(args, "shell", "getprop qemu.drape_benchmark").strip() != nonce:
        raise RuntimeError("Emulator ownership token does not match the launched session")
    require_root(args)
    properties = {
        name: adb(args, "shell", "getprop " + name).strip()
        for name in ("ro.build.version.sdk", "ro.product.cpu.abi", "ro.build.fingerprint", "ro.sf.lcd_density")}
    display = adb(args, "shell", "wm size; wm density")
    cpu = adb(args, "shell", "cat /sys/devices/system/cpu/present").strip()
    memory = adb(args, "shell", "cat /proc/meminfo")
    environment = {"properties": properties, "display": display, "cpu_present": cpu, "memory": memory,
                   "host": {"platform": sys.platform,
                            "machine": os.uname().machine if hasattr(os, "uname") else "unknown"},
                   "serial": args.serial, "owned_read_only_session": nonce is not None}
    (args.output / "environment.json").write_text(json.dumps(environment, indent=2) + "\n")
    for setting in ("auto_time", "auto_time_zone"):
        adb(args, "shell", "settings put global " + setting + " 0")
    adb(args, "shell", "settings put system screen_off_timeout 2147483647")
    with (args.output / "install.txt").open("w") as installation:
        def package_command(*command):
            try:
                output = adb(args, *command, timeout=120)
            except subprocess.CalledProcessError as exc:
                installation.write(exc.stdout or str(exc))
                raise
            installation.write(output)
            installation.flush()
            # Older adb/Android combinations return exit code 0 even on install failure.
            if "Success" not in output.splitlines():
                raise RuntimeError(f"adb {command[0]} failed; see install.txt")

        # A clean replacement also avoids stale permission/signature conflicts on old Android.
        if adb(args, "shell", "pm path " + PACKAGE).strip().startswith("package:"):
            package_command("uninstall", PACKAGE)
        package_command("install", str(args.apk))
    adb(args, "shell", "am force-stop " + PACKAGE)
    if adb(args, "shell", "pm clear " + PACKAGE).strip() != "Success":
        raise RuntimeError("Could not clear benchmark app data")
    package = adb(args, "shell", "dumpsys package " + PACKAGE)
    uid_match = re.search(r"\buserId=(\d+)\b", package)
    if not uid_match:
        raise RuntimeError("Cannot determine benchmark app UID")
    uid = uid_match[1]
    files, prefs = "/data/data/" + PACKAGE + "/files", "/data/data/" + PACKAGE + "/shared_prefs"
    map_directory = files + "/" + args.map_version
    adb(args, "shell", f"mkdir -p {map_directory} {prefs}")
    for source, target in ((args.map, map_directory + "/" + MAP_NAME),
                           (args.output / "settings.ini", files + "/settings.ini"),
                           (args.output / "scenario.json", files + "/graphics_benchmark.json"),
                           (args.output / "OrganicMapsPrefs.xml", prefs + "/OrganicMapsPrefs.xml")):
        adb(args, "push", str(source), target, timeout=120)
    adb(args, "shell", f"chown -R {uid}:{uid} {files} {prefs}; restorecon -R {files} {prefs}")
    adb(args, "shell", "input keyevent KEYCODE_WAKEUP; input keyevent KEYCODE_MENU")
    adb(args, "logcat", "-c")
    (args.output / "package.txt").write_text(package)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--apk", type=Path, required=True, help="Signed, non-debuggable Release benchmark APK")
    parser.add_argument("--output", type=Path,
                        default=Path("out") / ("drape-stress-" + dt.datetime.now().strftime("%Y%m%d-%H%M%S")))
    parser.add_argument("--sdk", type=Path, default=Path(
        os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME") or
        (str(Path.home() / "Library/Android/sdk") if sys.platform == "darwin" else str(Path.home() / "Android/Sdk"))))
    for tool in ("adb", "emulator", "aapt"):
        parser.add_argument("--" + tool, help=f"Override Android SDK {tool} executable")
    parser.add_argument("--avd", default="Nexus_6")
    timing = parser.add_mutually_exclusive_group()
    timing.add_argument("--random-delays", action="store_true",
                        help="Diagnostic jumps every 50–500 ms (seed 0); disable original-reference gates")
    timing.add_argument("--wait-for-ready", action="store_true",
                        help="Wait for each view's tiles and presented frame; report loading throughput and latency")
    parser.add_argument("--distributed-points", action="store_true",
                        help="Use the 500 map-wide targets with timed stress; disable original-reference gates")
    parser.add_argument("--serial", help="Use an existing emulator-NNNN; requires --reset-benchmark-app")
    parser.add_argument("--reset-benchmark-app", action="store_true",
                        help="Permit destructive fixture reset on --serial emulator")
    args = parser.parse_args()
    if os.name != "posix":
        parser.error("This emulator runner currently supports macOS and Linux")
    if (bool(args.serial) != args.reset_benchmark_app or
            (args.serial and not re.fullmatch(r"emulator-\d+", args.serial))):
        parser.error("--serial requires --reset-benchmark-app and an emulator-NNNN serial; "
                     "physical devices are refused")
    args.apk, args.output = (path.expanduser().resolve() for path in (args.apk, args.output))
    args.package, args.interval = PACKAGE, 0.5
    emulator = emulator_log = None
    created_output = prepared = False
    owned = args.serial is None
    try:
        if not args.apk.is_file():
            raise RuntimeError("APK must exist")
        scenario = workload(args.random_delays, args.wait_for_ready, args.distributed_points)
        args.duration_max = 1800 if args.wait_for_ready else sum(
            timing["duration_s"] for timing in scenario_timings(json.loads(scenario)["scenarios"])) + 100
        tools_from_sdk(args)
        manifest = verify_apk(args)
        args.map = get_map_path(args.map_version)
        args.output.mkdir(parents=True, exist_ok=False)
        created_output = True
        (args.output / "scenario.json").write_bytes(scenario)
        (args.output / "settings.ini").write_text(SETTINGS)
        (args.output / "OrganicMapsPrefs.xml").write_text('<?xml version="1.0" encoding="utf-8" standalone="yes" ?>\n'
            '<map><boolean name="EnableLogging" value="false" />'
            '<boolean name="FirstStartDialogSeen" value="true" /></map>\n')
        (args.output / "manifest.txt").write_text(manifest)
        inputs = {"apk": str(args.apk), "map": str(args.map),
                  "reference": (None if args.random_delays or args.wait_for_ready or args.distributed_points
                                else REFERENCE),
                  "package": PACKAGE, "arguments": vars(args).copy()}
        (args.output / "input.json").write_text(json.dumps(inputs, indent=2, default=str) + "\n")
        nonce = None
        if owned:
            port = free_emulator_port(args)
            args.serial, nonce = f"emulator-{port}", uuid.uuid4().hex
            command = [args.emulator, "-avd", args.avd, "-port", str(port), "-read-only", "-no-snapshot",
                       "-no-boot-anim", "-no-audio",
                       "-prop", "qemu.drape_benchmark=" + nonce]
            (args.output / "emulator-command.json").write_text(json.dumps(command, indent=2) + "\n")
            emulator_log = (args.output / "emulator.log").open("w")
            emulator = subprocess.Popen(command, stdout=emulator_log, stderr=subprocess.STDOUT, start_new_session=True)
        wait_for_boot(args, emulator)
        prepare(args, nonce)
        prepared = True
        time.sleep(5)
        capture(args)
        result = evaluate(args.output)
        return 0 if result["capture_valid"] and result["passed"] is not False else 2
    except (OSError, ValueError, RuntimeError, subprocess.SubprocessError, zipfile.BadZipFile, KeyError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        if created_output:
            (args.output / "error.txt").write_text(str(exc) + "\n")
        return 1
    finally:
        if emulator is not None:
            if emulator.poll() is None:
                os.killpg(emulator.pid, signal.SIGTERM)
                try:
                    emulator.wait(timeout=15)
                except subprocess.TimeoutExpired:
                    os.killpg(emulator.pid, signal.SIGKILL)
                    emulator.wait(timeout=5)
        elif not owned and prepared:
            try:
                adb(args, "shell", "am force-stop " + PACKAGE)
            except subprocess.SubprocessError:
                pass
        if emulator_log is not None:
            emulator_log.close()


if __name__ == "__main__":
    sys.exit(main())
