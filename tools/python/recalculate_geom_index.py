#!/usr/bin/env python3
"""Reindex writable maps using the shared edited styles, then relaunch on success.

recalculate_geom_index.py <resources_dir> <writable_dir> <generator_tool> [<app> <args>...]
Bundled World must be copied to its dated writable map directory before this script runs.
"""

import os
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from tempfile import TemporaryDirectory

WORKERS = 8
EXCLUDE_NAMES = {"WorldCoasts.mwm", "WorldCoasts_migrate.mwm"}


def find_all_mwms(data_path):
    # Scan maps directly in version directories, including directory symlinks.
    mwms = {}
    root = Path(data_path).resolve()
    for directory in root.iterdir():
        # Match platform::ParseVersion(): one to six ASCII digits.
        name = directory.name
        if not (
            len(name) <= 6 and name.isascii() and name.isdigit() and directory.is_dir()
        ):
            continue
        for p in directory.glob("*.mwm"):
            if p.name not in EXCLUDE_NAMES and p.name.endswith(".mwm") and p.is_file():
                # Keep paths under the writable root for --output; process each symlink target once.
                mwms.setdefault(p.resolve(), p)
    return sorted(mwms.values())


def process_mwm(generator_tool, mwm, resources_dir, writable_dir):
    print(f"Processing {mwm}", flush=True)
    output = mwm.relative_to(Path(writable_dir).resolve()).with_suffix("")
    # Duplicate country names across versions must never share an index temporary file.
    with TemporaryDirectory(prefix="designer-index-") as tmp:
        # generator_tool also uses --output as the relative index temporary-file prefix.
        (Path(tmp) / output).parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(
            [
                generator_tool,
                f"--data_path={writable_dir}",
                f"--user_resource_path={resources_dir}",
                f"--output={output}",
                "--generate_index=true",
                f"--intermediate_data_path={tmp}{os.sep}",
            ],
            check=True,
        )


def main(argv=None):
    args = sys.argv[1:] if argv is None else argv
    if len(args) < 3:
        print(__doc__, file=sys.stderr)
        return 1
    resources_dir, writable_dir, generator_tool, *relaunch = args
    try:
        mwms = find_all_mwms(writable_dir)
        with ThreadPoolExecutor(max_workers=WORKERS) as executor:
            list(
                executor.map(
                    lambda mwm: process_mwm(
                        generator_tool, mwm, resources_dir, writable_dir
                    ),
                    mwms,
                )
            )
        if relaunch:
            # The caller closes our output pipes when we exit; the relaunched app needs independent streams.
            devnull = subprocess.DEVNULL
            subprocess.Popen(relaunch, stdin=devnull, stdout=devnull, stderr=devnull)
    except (OSError, subprocess.CalledProcessError) as error:
        print(error, file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
