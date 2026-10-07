#!/usr/bin/env python3
"""Remove exact zero-area border retraces without changing filled coverage.

Only identical-endpoint or horizontal/vertical reversals are canceled. These
lines remain collinear in Mercator. No coordinates are moved or introduced,
and no simplification is run. Geometry repairs are used for measurements only;
their coordinates are never written to the candidate.
"""

import argparse
import json
import logging
from pathlib import Path
import shutil

import numpy as np
from shapely import make_valid
from shapely.geometry import GeometryCollection, MultiPolygon, Polygon
from shapely.ops import unary_union
from shapely.validation import explain_validity

from maintain_borders import Border, Ring, border_geometry, prepare_validity, project, read_poly


LOG = logging.getLogger(__name__)


def cancel_backtracks(points):
    """Cancel opposite traversals of an existing line, preserving other records.

    A-B-C is a retrace when A equals C, or all three coordinates share longitude
    or latitude and B lies outside A-C. Opposite edge/ray crossings cancel, so
    replacing them by A-C preserves the generator's even/odd polygon fill.
    """
    retained = np.arange(len(points))
    closed = bool(np.array_equal(points[0], points[-1]))
    operations = []
    while True:
        selected = points[retained]
        different = np.r_[True, np.any(selected[1:] != selected[:-1], axis=1)]
        clean = selected[different]
        groups = np.cumsum(different) - 1
        if len(clean) > 1 and np.array_equal(clean[0], clean[-1]):
            groups[groups == len(clean) - 1] = 0
            clean = clean[:-1]
        if len(clean) <= 3:
            break
        previous = np.roll(clean, 1, axis=0)
        following = np.roll(clean, -1, axis=0)
        identical = np.all(previous == following, axis=1)
        horizontal = (previous[:, 1] == clean[:, 1]) & (following[:, 1] == clean[:, 1])
        vertical = (previous[:, 0] == clean[:, 0]) & (following[:, 0] == clean[:, 0])
        reversing = np.sum((previous - clean) * (following - clean), axis=1) > 0
        remove = np.flatnonzero(identical | ((horizontal | vertical) & reversing))
        if not len(remove):
            break
        keep = ~np.isin(groups, remove)
        if len(np.unique(selected[keep], axis=0)) < 3:
            return points, []
        for index in remove:
            operations.append({
                "previous": previous[index].tolist(), "removed": clean[index].tolist(),
                "following": following[index].tolist(),
                "type": "identical-endpoint retrace" if identical[index] else "axis-aligned partial retrace",
                "removed_original_record_indices": retained[groups == index].tolist(),
            })
        retained = retained[keep]
    if not operations:
        return points, []
    result = points[retained]
    if closed and not np.array_equal(result[0], result[-1]):
        result = np.r_[result, result[:1]]
    return result, operations


def areal_geometry(geometry):
    """Discard only zero-area components from temporary measurement geometry."""
    if isinstance(geometry, (Polygon, MultiPolygon)):
        return geometry
    if hasattr(geometry, "geoms"):
        parts = [areal_geometry(part) for part in geometry.geoms]
        return unary_union([part for part in parts if not part.is_empty])
    return GeometryCollection()


def measured_geometry(border):
    geometry = border_geometry(border, [project(ring.points) for ring in border.rings])
    if geometry is None:
        raise ValueError(f"Cannot assign holes in {border.name}")
    return areal_geometry(geometry if geometry.is_valid else make_valid(geometry))


def repair_borders(borders):
    """Return exact-source repairs only after unchanged areal coverage is proved."""
    if not borders:
        raise ValueError("No borders provided")
    invalid_before, invalid_files_before = prepare_validity(borders)
    output = {}
    repairs = []
    candidates = []
    changed = []
    for border in borders:
        points = []
        for ring in border.rings:
            repaired, operations = cancel_backtracks(ring.points) if not ring.valid else (ring.points, [])
            points.append(repaired)
            if operations:
                repairs.append({
                    "file": border.name, "ring": ring.name, "operations": operations,
                    "records_before": len(ring.points), "records_after": len(repaired),
                    "valid_after": Polygon(project(repaired)).is_valid,
                    "validity_before": explain_validity(Polygon(project(ring.points))),
                    "validity_after": explain_validity(Polygon(project(repaired))),
                })
        output[border.name] = points
        candidates.append(Border(border.name, border.title, [
            Ring(ring.name, selected) for ring, selected in zip(border.rings, points)
        ]))
        if any(not np.array_equal(ring.points, selected) for ring, selected in zip(border.rings, points)):
            changed.append(border.name)
    invalid_after, invalid_files_after = prepare_validity(candidates)
    original_invalid = {(item["file"], item["ring"]) for item in invalid_before}
    newly_invalid = [item for item in invalid_after if (item["file"], item["ring"]) not in original_invalid]
    newly_invalid_files = sorted(set(invalid_files_after) - set(invalid_files_before))
    if newly_invalid or newly_invalid_files:
        raise ValueError(f"Retrace cancellation introduced invalid geometry: {newly_invalid}, {newly_invalid_files}")
    proofs = []
    for before, after in zip(borders, candidates):
        if before.name not in changed:
            continue
        if not measured_geometry(before).equals(measured_geometry(after)):
            raise ValueError(f"Retrace cancellation changed areal coverage in {before.name}")
        proofs.append({"file": before.name, "areal_geometry_topologically_equal": True})
    report = {
        "algorithm": "Cancel exact opposite traversals only; no simplification, snapping or new coordinates",
        "coverage_proof": "Even/odd ray crossings cancel; every changed file's projected areal geometry is topologically equal, so all areal peer intersections are unchanged",
        "files": len(borders), "changed_files": changed,
        "source_invalid_rings": invalid_before, "candidate_invalid_rings": invalid_after,
        "source_invalid_files": invalid_files_before, "candidate_invalid_files": invalid_files_after,
        "repaired_rings_now_valid": sum(repair["valid_after"] for repair in repairs),
        "removed_coordinate_records": sum(repair["records_before"] - repair["records_after"] for repair in repairs),
        "repairs": repairs, "file_proofs": proofs,
    }
    return output, report


def write_retained_poly(path, border, points):
    """Retain source coordinate text, blank lines and metadata byte for byte."""
    if border.source is None or len(points) != len(border.rings):
        raise ValueError("Original source and matching ring count are required")
    lines = border.source.read_bytes().decode("utf-8").splitlines(keepends=True)
    if not lines or lines[0].rstrip("\r\n") != border.title:
        raise ValueError(f"Source title changed in {border.name}")
    keep = np.ones(len(lines), dtype=bool)
    closures = {}
    position = 1
    for ring, retained in zip(border.rings, points):
        while position < len(lines) and not lines[position].strip():
            position += 1
        if position == len(lines) or lines[position].strip() != ring.name:
            raise ValueError(f"Source ring metadata changed in {border.name}/{ring.name}")
        position += 1
        first = position
        while position < len(lines) and lines[position].strip() != "END":
            position += 1
        if position == len(lines):
            raise ValueError(f"Unclosed source ring in {border.name}/{ring.name}")
        records = [index for index in range(first, position) if lines[index].strip()]
        original = np.fromstring(" ".join(lines[index] for index in records), dtype=float, sep=" ")
        if original.size != ring.points.size or not np.array_equal(original.reshape(-1, 2), ring.points):
            raise ValueError(f"Source coordinates changed in {border.name}/{ring.name}")
        if not np.array_equal(retained, ring.points):
            keep[records] = False
            cursor = 0
            first_record = None
            for index, point in enumerate(retained):
                while cursor < len(ring.points) and not (
                    ring.points[cursor, 0] == point[0] and ring.points[cursor, 1] == point[1]
                ):
                    cursor += 1
                if cursor == len(ring.points):
                    # Canceling the original starting point can require a new
                    # closing record. Copy the retained first record exactly.
                    if index == len(retained) - 1 and first_record is not None and np.array_equal(point, retained[0]):
                        closures[position] = lines[first_record]
                        continue
                    raise ValueError(f"Output is not a source subsequence in {border.name}/{ring.name}")
                record = records[cursor]
                keep[record] = True
                if first_record is None:
                    first_record = record
                cursor += 1
        position += 1
    while position < len(lines) and not lines[position].strip():
        position += 1
    if position == len(lines) or lines[position].strip() != "END" or any(line.strip() for line in lines[position + 1:]):
        raise ValueError(f"Source final END changed in {border.name}")
    text = "".join(closures.get(index, "") + (line if keep[index] else "") for index, line in enumerate(lines))
    path.write_bytes(text.encode("utf-8"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    source, candidate = args.input.resolve(), args.output.resolve()
    if source == candidate or source in candidate.parents or candidate in source.parents:
        parser.error("Input and output directories must be separate, without containing each other")
    if candidate.exists() and any(candidate.iterdir()):
        parser.error("Output directory must be empty or absent")
    paths = sorted(source.glob("*.poly"))
    if not paths:
        parser.error("No .poly files found")
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    LOG.info("Reading %s border files", len(paths))
    borders = [read_poly(path) for path in paths]
    output, report = repair_borders(borders)
    candidate.mkdir(parents=True, exist_ok=True)
    changed = set(report["changed_files"])
    for border in borders:
        if border.name in changed:
            write_retained_poly(candidate / border.name, border, output[border.name])
        else:
            shutil.copyfile(border.source, candidate / border.name)
    (candidate / "backtrack-repair-report.json").write_text(json.dumps(report, indent=2) + "\n")
    LOG.info("Removed %s records in %s files; %s invalid rings became valid", report["removed_coordinate_records"],
             len(changed), report["repaired_rings_now_valid"])


if __name__ == "__main__":
    main()
