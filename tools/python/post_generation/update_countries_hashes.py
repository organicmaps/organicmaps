#!/usr/bin/env python3
"""
Recalculate per-map integrity hashes in a countries.json file.

For every node in countries.json that has a corresponding <id>.mwm file in the
given directory, (re)compute its BLAKE3 hash (truncated, base64) and store it in
the node's "h" field. Any legacy "sha1_base64" field is removed.

This covers both use cases:
  * migrate an old countries.json (SHA-1 "sha1_base64") to BLAKE3 "h";
  * recalculate / add BLAKE3 "h" values for a freshly built set of mwm files.

The hash length and algorithm match the client (coding::Blake3::CalculateMwmBase64)
and the generator (hierarchy_to_countries.py). The file is
rewritten with the generator's formatting (json.dump(ensure_ascii=False,
indent=1)), so only the hash lines change.

Examples:
  # In place, countries.json sitting next to the mwm files:
  python update_countries_hashes.py --mwm /maps/250101

  # Explicit input/output:
  python update_countries_hashes.py --mwm /maps/250101 \\
      --countries data/countries.json -o data/countries.json
"""

import argparse
import json
import os
import sys

# Allow running both directly and as `python -m post_generation.update_countries_hashes`.
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))

from post_generation.hierarchy_to_countries import (
    HASH_KEY,
    HASH_NUM_BYTES,
    LEGACY_HASH_KEY,
    file_hash_base64,
)


def update_tree(node, mwm_dir, num_bytes, stats):
    """Recursively (re)compute the hash for every node backed by an mwm file."""
    name = node.get("id")
    if name:
        mwm_path = os.path.join(mwm_dir, f"{name}.mwm")
        if os.path.isfile(mwm_path):
            node[HASH_KEY] = file_hash_base64(mwm_path, num_bytes)
            stats["updated"] += 1
            if LEGACY_HASH_KEY in node:
                del node[LEGACY_HASH_KEY]
                stats["migrated"] += 1
        elif HASH_KEY in node or LEGACY_HASH_KEY in node:
            # The node carries a hash but its mwm is missing here: leave it as is.
            stats["missing"].append(name)

    for child in node.get("g", []):
        update_tree(child, mwm_dir, num_bytes, stats)


def main():
    parser = argparse.ArgumentParser(
        description="Recalculate/migrate per-map integrity hashes in countries.json to BLAKE3."
    )
    parser.add_argument(
        "--mwm",
        "--target",
        dest="mwm",
        required=True,
        help="Path to the directory with .mwm files",
    )
    parser.add_argument(
        "--countries",
        help="Path to the input countries.json (default: <mwm>/countries.json)",
    )
    parser.add_argument(
        "-o",
        "--output",
        help="Output path (default: overwrite the input countries.json)",
    )
    parser.add_argument(
        "--hash-bytes",
        type=int,
        default=HASH_NUM_BYTES,
        help=f"Truncated hash length in bytes (default: {HASH_NUM_BYTES})",
    )
    args = parser.parse_args()

    countries_path = args.countries or os.path.join(args.mwm, "countries.json")
    output_path = args.output or countries_path

    with open(countries_path, encoding="utf-8") as f:
        root = json.load(f)

    stats = {"updated": 0, "migrated": 0, "missing": []}
    update_tree(root, args.mwm, args.hash_bytes, stats)

    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(root, f, ensure_ascii=False, indent=1)

    print(
        f"Wrote {output_path}: updated {stats['updated']} hash(es), "
        f"removed {stats['migrated']} legacy '{LEGACY_HASH_KEY}' field(s)."
    )
    if stats["missing"]:
        print(
            f"WARNING: {len(stats['missing'])} node(s) have a hash but no matching "
            f".mwm in {args.mwm} (left unchanged):",
            file=sys.stderr,
        )
        for name in stats["missing"]:
            print(f"  {name}", file=sys.stderr)


if __name__ == "__main__":
    main()
