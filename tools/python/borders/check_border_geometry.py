#!/usr/bin/env python3
"""Fail on source or decoded packed geometry regressions against an explicit baseline.

A region split or rename is allowed when its rings are valid and global coverage
and physical ambiguity are preserved. Areas use the audit tool's approximate
latitude correction. This command never generates or writes border data.
"""

import argparse
import hashlib
import json
import logging
import math
from pathlib import Path

from shapely.geometry import GeometryCollection, MultiPolygon, Polygon, box
from shapely.ops import unary_union
from shapely.validation import explain_validity

from border_comparison import (Dataset, area_m2, compare, cross_ring_contacts, finding, point_contacts_fit,
                               ring_contact_kinds, source_dataset)
from border_geometry import measurement_difference, ring_coverage
from packed_borders import PackedBorders, mercator


LOG = logging.getLogger(__name__)


def rectangle_mask(bounds):
    left, bottom, right, top = bounds
    # CountryDef::IsPointInsideRect accepts all three canonical longitude shifts.
    return unary_union([box(left + shift, bottom, right + shift, top) for shift in (-360, 0, 360)])


def packed_datasets(path, bounds_format="wrapped"):
    packed = PackedBorders(path, bounds_format)
    names, shapes, invalid_rings, invalid_files = [], [], [], []
    contacts, file_contacts, contact_kinds, clipped = {}, {}, {}, {}
    path_points = 0
    for index, name in enumerate(packed.names):
        filename = name + ".poly"
        names.append(filename)
        parts, polygons, rings = [], [], []
        for ring_index, integers in enumerate(packed.paths(index), 1):
            points = mercator(integers)
            path_points += len(points)
            ring_name = str(ring_index)
            polygon = Polygon(points) if len(points) >= 3 else Polygon()
            valid = polygon.is_valid and not polygon.is_empty
            if len(points) >= 3:
                rings.append((ring_name, points, False, valid))
            polygons.append(polygon)
            if not valid:
                invalid_rings.append({"file": filename, "ring": ring_name, "reason": explain_validity(polygon)})
                key = filename, ring_name
                if len(points) >= 3:
                    contact_kinds[key] = ring_contact_kinds(points)
                    contacts[key] = unary_union(list(contact_kinds[key].values()))
                else:
                    contacts[key] = GeometryCollection()
            parts.append(ring_coverage(points, valid) if len(points) >= 3 else Polygon())
        assembled = MultiPolygon(polygons) if len(polygons) > 1 else polygons[0]
        if not assembled.is_valid or assembled.is_empty:
            invalid_files.append(filename)
            for pair, contact in cross_ring_contacts(rings).items():
                file_contacts[filename, *pair] = contact
        measured = unary_union(parts)
        mask = rectangle_mask(packed.bounds[name])
        clipped[filename] = measured.difference(mask)
        shapes.append(measured.intersection(mask))
        if index % 100 == 0:
            LOG.info("Decoded outer paths for %s/%s regions", index, len(packed.names))
    metadata = {"sha256": hashlib.sha256(packed.data).hexdigest(), "size_bytes": len(packed.data),
                "bounds_format": bounds_format,
                "files": len(names), "path_points": path_points,
                "bounds": packed.bounds}
    outer = Dataset(names, shapes, invalid_rings, invalid_files, metadata, contacts, file_contacts, contact_kinds)
    return outer, clipped


def check(source, packed_path, baseline_source, baseline_packed_path, minimum_area=0.01,
          bounds_format="wrapped", baseline_bounds_format="wrapped"):
    LOG.info("Reading current source borders: %s", source)
    current_source = source_dataset(source)
    LOG.info("Reading baseline source borders: %s", baseline_source)
    old_source = source_dataset(baseline_source)
    LOG.info("Reading current packed geometry: %s", packed_path)
    current, clips = packed_datasets(packed_path, bounds_format)
    LOG.info("Reading baseline packed geometry: %s", baseline_packed_path)
    old, old_clips = packed_datasets(baseline_packed_path, baseline_bounds_format)
    if set(current.names) != set(current_source.names):
        raise ValueError("Packed names must match the current .poly filenames")
    if set(old.names) != set(old_source.names):
        raise ValueError("Baseline packed names must match the baseline .poly filenames")
    LOG.info("Comparing source geometry against baseline")
    source_report = compare(current_source, old_source, minimum_area)
    LOG.info("Comparing packed outer paths against baseline")
    path_report = compare(current, old, minimum_area)
    report = {"minimum_area_m2": minimum_area,
              "measurement": "Original ring even/odd fill, OR across outers, Mercator segments; physical ambiguity compares owner multiplicity; packed coverage uses wrapped CountryDef bounds",
              "source": source_report,
              "packed_paths": path_report,
              "current_source_metadata": current_source.metadata, "baseline_source_metadata": old_source.metadata,
              "current_packed_metadata": current.metadata, "baseline_packed_metadata": old.metadata}
    new_clips = []
    for name, geometry in clips.items():
        added = measurement_difference(geometry, old_clips.get(name, Polygon()))
        if area_m2(added) > minimum_area:
            new_clips.append(finding(added, file=name))
    old_unclosed = {(item["file"], item["ring"]) for item in old_source.metadata["unclosed_rings"]}
    new_unclosed = [item for item in current_source.metadata["unclosed_rings"] if (item["file"], item["ring"]) not in old_unclosed]
    report.update(new_metadata_clipping=new_clips, new_unclosed_source_rings=new_unclosed)
    report["regressions"] = any(report[label]["regressions"] for label in ("source", "packed_paths")) or bool(
        new_clips or new_unclosed)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--packed", type=Path, required=True)
    parser.add_argument("--baseline-source", type=Path, required=True)
    parser.add_argument("--baseline-packed", type=Path, required=True)
    parser.add_argument("--bounds-format", choices=("legacy", "wrapped"), default="wrapped",
                        help="Current CountryDef serialization: legacy signed or wrapped unsigned bounds")
    parser.add_argument("--baseline-bounds-format", choices=("legacy", "wrapped"), default="wrapped")
    parser.add_argument("--minimum-area", type=float, default=0.01)
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if not math.isfinite(args.minimum_area) or args.minimum_area < 0:
        parser.error("Minimum area must be finite and nonnegative")
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    report = check(args.source, args.packed, args.baseline_source, args.baseline_packed, args.minimum_area,
                   args.bounds_format, args.baseline_bounds_format)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    if report["regressions"]:
        for label in ("source", "packed_paths"):
            for category in ("new_invalid_rings", "new_invalid_files", "new_invalid_ring_contacts",
                             "new_invalid_file_contacts", "introduced_overlap_parts", "introduced_uncovered_parts"):
                findings = report[label][category]
                if findings:
                    examples = [item if isinstance(item, str) else item.get("file", "/".join(item.get("owners", [])))
                                for item in findings[:5]]
                    LOG.error("%s: %s: %s (%s)", label, category, len(findings), ", ".join(examples))
        for category in ("new_metadata_clipping", "new_unclosed_source_rings"):
            if report[category]:
                LOG.error("%s: %s", category, len(report[category]))
        LOG.error("Border geometry regressions; details: %s", args.report)
    else:
        LOG.info("No border geometry regressions; report: %s", args.report)
    raise SystemExit(1 if report["regressions"] else 0)


if __name__ == "__main__":
    main()
