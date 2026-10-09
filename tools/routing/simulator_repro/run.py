#!/usr/bin/env python3
"""Install and verify the isolated probes on an already booted simulator."""
import argparse
import subprocess
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--device", required=True, help="UDID of a booted iOS simulator")
parser.add_argument("--repeat", type=int, default=1)
args = parser.parse_args()
directory = Path(__file__).resolve().parent

for mode in ["baseline", "fixed"]:
    bundle = directory / mode / "RoutingRepro.app"
    subprocess.run(["xcrun", "simctl", "install", args.device, str(bundle)], check=True)
    for repetition in range(1, args.repeat + 1):
        command = ["xcrun", "simctl", "launch", "--console", "--terminate-running-process", args.device,
                   "app.organicmaps.routing-repro." + mode]
        if mode == "fixed":
            command.append("--fixed")
        result = subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, timeout=60)
        log = directory / f"{mode}-run-{repetition}.log"
        log.write_text(result.stdout)
        evidence = [line for line in result.stdout.splitlines()
                    if line.startswith(("SIMULATOR_REPRO", "ISSUE "))]
        print(f"{mode}, run {repetition}: " + str(log), flush=True)
        print("\n".join(evidence), flush=True)
        if result.returncode or "SIMULATOR_REPRO PASS: 4 issues plus reset control" not in result.stdout:
            print(result.stdout[-10000:], flush=True)
            raise SystemExit("Probe did not complete; inspect its log and the simulator crash report")
