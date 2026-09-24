#!/usr/bin/env python3
"""Generate rapid viewport scenarios and summarize DRAPE_QUEUE_TRACE logs."""

import argparse
import json
from pathlib import Path


def generate(args):
    def center(lat, lon, zoom):
        return {
            "actionType": "centerViewport",
            "center": {"lat": lat, "lon": lon},
            "zoomLevel": zoom,
            "animated": False,
        }

    def wait(milliseconds):
        return {"actionType": "waitForTime", "timeMs": milliseconds}

    scenarios = []
    for name in ("same-zoom-pan", "zoom-changes"):
        steps = [center(args.lat, args.lon, args.zoom), wait(5000)]
        for i in range(args.steps):
            # Revisit A between moves, exercising reads that leave and reenter coverage.
            offset = (0, 1, 0, -1)[i % 4] * args.pan_degrees
            zoom = args.zoom if name == "same-zoom-pan" else (args.zoom, args.min_zoom, args.max_zoom)[i % 3]
            steps += [center(args.lat, args.lon + offset, zoom), wait(args.interval_ms)]
        steps += [center(args.lat, args.lon, args.zoom), wait(args.drain_ms)]
        scenarios.append({"name": name, "steps": steps})
    args.output.write_text(json.dumps({"scenarios": scenarios}, indent=2) + "\n")
    duration = 2 * (5000 + args.steps * args.interval_ms + args.drain_ms) / 1000
    print(f"Wrote {args.output}; scripted duration {duration:g}s plus map loading.")


def summarize(args):
    for path in args.logs:
        samples = []
        finished = False
        for line in path.read_text(errors="replace").splitlines():
            if "Drape benchmark finished" in line:
                finished = True
                break
            marker = "DrapeQueue "
            if marker in line:
                samples.append(json.loads(line.split(marker, 1)[1]))
        if not samples:
            raise SystemExit(f"No DrapeQueue samples in {path}; build with DRAPE_QUEUE_TRACE=ON.")
        last = samples[-1]
        print(path)
        print("  endpoint: last sample before benchmark finish" if finished else "  endpoint: last available sample")
        print(f"  exact peak: {max(s['peak'] for s in samples)}, last size: {last['size']}, "
              f"samples: {len(samples)}")
        print("  type                         enqueued   popped filtered rejected cleared  peak pending")
        for name, counts in sorted(last["types"].items(), key=lambda item: item[1]["enqueued"], reverse=True):
            print(f"  {name:28s} {counts['enqueued']:8d} {counts['popped']:8d} "
                  f"{counts['filtered']:8d} {counts['rejected']:8d} {counts['cleared']:7d} "
                  f"{counts['peak']:5d} {counts['size']:7d}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    scenario = commands.add_parser("generate", help="Write graphics_benchmark.json")
    scenario.add_argument("--output", type=Path, required=True)
    scenario.add_argument("--lat", type=float, default=51.509865)
    scenario.add_argument("--lon", type=float, default=-0.118092)
    scenario.add_argument("--zoom", type=int, default=15)
    scenario.add_argument("--min-zoom", type=int, default=10)
    scenario.add_argument("--max-zoom", type=int, default=17)
    scenario.add_argument("--steps", type=int, default=500)
    scenario.add_argument("--interval-ms", type=int, default=50)
    scenario.add_argument("--pan-degrees", type=float, default=0.08)
    scenario.add_argument("--drain-ms", type=int, default=10000)
    scenario.set_defaults(run=generate)
    report = commands.add_parser("summarize", help="Report exact peaks and cumulative message counts")
    report.add_argument("logs", type=Path, nargs="+")
    report.set_defaults(run=summarize)
    args = parser.parse_args()
    args.run(args)


if __name__ == "__main__":
    main()
