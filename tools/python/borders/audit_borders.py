#!/usr/bin/env python3
"""Audit .poly geometry and compare a candidate directory with its sources.

Requires NumPy and Shapely. Invalid rings are measured with even/odd filling;
this tool never writes polygons. Checks use the generator's degrees-scaled
Mercator segments. Areas are corrected for latitude, suitable for finding
slivers, not surveying; reported bounds and points are longitude/latitude.
"""

import argparse
import json
import logging
import math
from pathlib import Path

from shapely import STRtree
from shapely.geometry import Polygon
from shapely.ops import unary_union

from border_comparison import (area_m2, compare, geographic_bounds, geographic_point, load,
                               source_dataset, uncovered_loss)
from border_geometry import even_odd_contains, ring_coverage


LOG = logging.getLogger(__name__)


def candidate_pairs(shapes):
    tree = STRtree(shapes)
    return {(i, int(j)) for i, shape in enumerate(shapes) for j in tree.query(shape) if j > i}


def interior_holes(shapes):
    union = unary_union(shapes)
    parts = union.geoms if hasattr(union, "geoms") else [union]
    return [Polygon(ring) for part in parts if isinstance(part, Polygon) for ring in part.interiors]


def audit(directory, baseline=None, groups=(), minimum_area=1.0):
    borders, shapes, report = load(directory)
    names = [border.name for border in borders]
    pairs = candidate_pairs(shapes)
    coverage_losses = []
    if baseline:
        old_borders, old_shapes, old_report = load(baseline)
        if names != [border.name for border in old_borders]:
            raise ValueError("Candidate and baseline have different .poly file names")
        pairs |= candidate_pairs(old_shapes)
        current = source_dataset(directory, (borders, shapes, report))
        old = source_dataset(baseline, (old_borders, old_shapes, old_report))
        # Manual acceptance uses the same strict comparison as final validation.
        report.update(compare(current, old, minimum_area, coverage_losses))
        for part in report["introduced_overlap_parts"]:
            part["a"], part["b"] = part["owners"]
        report["baseline"] = old_report
        report["regressions"] |= bool(report["changed_metadata"] or report["unclosed_rings"])
    overlaps = []
    for number, (i, j) in enumerate(sorted(pairs)):
        intersection = shapes[i].intersection(shapes[j])
        area = area_m2(intersection)
        if area > minimum_area:
            overlaps.append({"a": names[i], "b": names[j], "approx_m2": area,
                             "bounds": geographic_bounds(intersection)})
        if number % 500 == 0:
            LOG.info("Checked %s/%s neighboring pairs", number, len(pairs))
    report["candidate_pairs"] = len(pairs)
    report["overlaps"] = sorted(overlaps, key=lambda item: -item["approx_m2"])
    global_tree = STRtree(shapes)
    if baseline:
        loss_by_name = dict(coverage_losses)
        for part in report["introduced_uncovered_parts"]:
            lost = loss_by_name[part["file"]]
            part["neighbors"] = [names[i] for i in global_tree.query(lost)
                                 if shapes[i].distance(lost) < 1e-9]
    loss_names = [name for name, geometry in coverage_losses]
    losses = [geometry for name, geometry in coverage_losses]
    loss_tree = STRtree(losses)
    holes = []
    new_holes = []
    for group in groups:
        selected = {name for name in names if group == "*" or name.startswith(group)}
        members = [shape for name, shape in zip(names, shapes) if name in selected]
        if not members:
            raise ValueError(f"No files match group {group}")
        for hole in interior_holes(members):
            area = area_m2(hole)
            if area > minimum_area:
                holes.append({"group": group, "approx_m2": area, "bounds": geographic_bounds(hole),
                              "point": geographic_point(hole)})
                if baseline:
                    # Reuse the strict coverage proof. Growing coverage can
                    # enclose an old exterior void without introducing a loss.
                    parts = [hole.intersection(losses[i]) for i in loss_tree.query(hole, predicate="intersects")
                             if loss_names[i] in selected]
                    added = unary_union(parts)
                    if area_m2(added) > minimum_area:
                        neighbors = [names[i] for i in global_tree.query(added)
                                     if shapes[i].distance(added) < 1e-9]
                        new_holes.append({"group": group, "approx_m2": area_m2(added),
                                          "bounds": geographic_bounds(added), "neighbors": neighbors})
        LOG.info("Checked interior holes for %s", group)
    report["interior_holes"] = sorted(holes, key=lambda item: -item["approx_m2"])
    if baseline:
        report["introduced_interior_holes"] = sorted(new_holes, key=lambda item: -item["approx_m2"])
    report["minimum_area_m2"] = minimum_area
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--baseline", type=Path)
    parser.add_argument("--group", action="append", default=[], help="Filename prefix for union hole checks; '*' for all")
    parser.add_argument("--minimum-area", type=float, default=1.0, help="Report areas above this many square meters")
    parser.add_argument("--report", type=Path, required=True)
    args = parser.parse_args()
    if not math.isfinite(args.minimum_area) or args.minimum_area < 0:
        parser.error("Minimum area must be finite and nonnegative")
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    report = audit(args.directory, args.baseline, args.group, args.minimum_area)
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    LOG.info("%s files, %s invalid rings, %s overlapping pairs; report: %s",
             report["files"], len(report["invalid_rings"]), len(report["overlaps"]), args.report)
    if args.baseline and report["regressions"]:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
