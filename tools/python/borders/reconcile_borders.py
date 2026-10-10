#!/usr/bin/env python3
"""Reconcile nearly identical shared arcs without simplifying them.

Match arcs between exact common vertices, then reuse the denser coordinate
sequence for all matching owners within the existing Mercator tolerance.
Density is a deterministic selection rule, not evidence of OSM accuracy:
this repairs coordinate agreement and does not refresh administrative borders.
Invalid source files lock their shared arcs; newly invalid output locks the
affected replacement in every owner. Wider discrepancies remain unchanged.
The tolerance uses a continuous Hausdorff upper bound: match normalized arc
length parameters and compare both piecewise linear paths at their combined
knots. Between knots the distance is convex, so its maximum is at a knot.
"""

import argparse
from collections import defaultdict
from dataclasses import dataclass
import json
import logging
from pathlib import Path
import shutil

import numpy as np
from shapely.geometry import LineString, Polygon

from maintain_borders import DEFAULT_EPSILON, Ring, border_geometry, prepare_validity, project, read_poly, write_poly


LOG = logging.getLogger(__name__)


@dataclass
class Chain:
    ring: Ring
    region: int
    index: int
    ids: np.ndarray
    anchors: np.ndarray
    closed: bool


@dataclass
class Arc:
    chain: int
    begin: int
    end: int
    region: int


def clean_ids(ids):
    return ids[np.r_[True, ids[1:] != ids[:-1]]]


def canonical(ids):
    ids = clean_ids(ids)
    return ids[::-1] if ids[0] > ids[-1] else ids


def endpoint_keys(ids, anchors):
    first = ids[anchors[:-1]]
    last = ids[anchors[1:]]
    return (np.minimum(first, last).astype(np.uint64) << np.uint64(32)) | np.maximum(first, last)


def arc_distance_upper_bound(first, second):
    """Bound continuous Hausdorff distance by matching normalized arc length."""
    def parameterize(points):
        lengths = np.r_[0.0, np.cumsum(np.linalg.norm(np.diff(points, axis=0), axis=1))]
        if lengths[-1] == 0:
            return np.array([0.0, 1.0]), np.repeat(points[:1], 2, axis=0)
        retained = np.r_[True, lengths[1:] != lengths[:-1]]
        return lengths[retained] / lengths[-1], points[retained]

    first_knots, first_points = parameterize(first)
    second_knots, second_points = parameterize(second)
    knots = np.union1d(first_knots, second_knots)
    offsets = np.column_stack([
        np.interp(knots, first_knots, first_points[:, axis])
        - np.interp(knots, second_knots, second_points[:, axis])
        for axis in (0, 1)
    ])
    return float(np.max(np.linalg.norm(offsets, axis=1)))


def normalize(borders, epsilon=DEFAULT_EPSILON, preserve_files=()):
    """Return per-file ring coordinates and an audit report without writing."""
    if not np.isfinite(epsilon) or epsilon <= 0:
        raise ValueError("Epsilon must be finite and positive")
    if not borders:
        raise ValueError("No borders provided")
    if len(borders) >= 2**16:
        raise ValueError("More than 65535 border files are not supported")
    preserved = {name if name.endswith(".poly") else name + ".poly" for name in preserve_files}
    unknown = preserved - {border.name for border in borders}
    if unknown:
        raise ValueError(f"Unknown preserved files: {', '.join(sorted(unknown))}")
    invalid_rings, invalid_files = prepare_validity(borders)
    rings = [ring for border in borders for ring in border.rings]
    sizes = np.array([len(ring.points) for ring in rings])
    LOG.info("Interning %s coordinate records", int(sizes.sum()))
    coordinates, inverse = np.unique(np.concatenate([ring.points for ring in rings]), axis=0, return_inverse=True)
    if len(coordinates) >= 2**32:
        raise ValueError("More than 2^32 distinct vertices are not supported")
    inverse = inverse.astype(np.uint32)
    minimum = np.full(len(coordinates), len(borders), dtype=np.uint16)
    maximum = np.zeros(len(coordinates), dtype=np.uint16)
    offset = 0
    for region, border in enumerate(borders):
        for ring in border.rings:
            size = len(ring.points)
            ring.ids = inverse[offset:offset + size]
            np.minimum.at(minimum, ring.ids, region)
            np.maximum.at(maximum, ring.ids, region)
            offset += size
    shared = minimum != maximum
    del minimum, maximum
    chains = []
    candidates = defaultdict(list)
    LOG.info("Finding arcs between %s shared coordinates", int(shared.sum()))
    for region, border in enumerate(borders):
        for index, ring in enumerate(border.rings):
            closed = bool(np.array_equal(ring.points[0], ring.points[-1]))
            ids = ring.ids[:-1] if closed else ring.ids
            anchors = np.flatnonzero(shared[ids])
            if len(anchors) < 2 or len(np.unique(ids[anchors])) < 2:
                continue
            start = int(anchors[0])
            chain_ids = np.r_[ids[start:], ids[:start], ids[start]]
            anchors = np.r_[anchors - start, len(ids)]
            chain = Chain(ring, region, index, chain_ids, anchors, closed)
            chain_index = len(chains)
            chains.append(chain)
            keys = endpoint_keys(chain_ids, anchors)
            # Most exact shared arcs are two-point edges. Store only pairs that
            # might conceal unequal segmentation; match short arcs below.
            for position in np.flatnonzero(np.diff(anchors) > 1):
                begin, end = map(int, anchors[position:position + 2])
                selected = canonical(chain_ids[begin:end + 1])
                if len(selected) > 2 and selected[0] != selected[-1]:
                    candidates[int(keys[position])].append(Arc(chain_index, begin, end, region))
    LOG.info("Matching short arcs against %s candidate endpoint pairs", len(candidates))
    candidate_keys = np.array(sorted(candidates), dtype=np.uint64)
    for chain_index, chain in enumerate(chains):
        keys = endpoint_keys(chain.ids, chain.anchors)
        positions = np.searchsorted(candidate_keys, keys)
        found = positions < len(candidate_keys)
        found[found] &= candidate_keys[positions[found]] == keys[found]
        for position in np.flatnonzero(found):
            begin, end = map(int, chain.anchors[position:position + 2])
            selected = clean_ids(chain.ids[begin:end + 1])
            if len(selected) <= 2:
                candidates[int(keys[position])].append(Arc(chain_index, begin, end, chain.region))
    projected = project(coordinates)
    accepted = {}
    skipped_far = []
    skipped_invalid = 0
    skipped_preserved = 0
    unequal_groups = 0
    group_details = []
    for key, arcs in candidates.items():
        owners = sorted({arc.region for arc in arcs})
        if len(owners) < 2:
            continue
        variants = {}
        for arc in arcs:
            ids = canonical(chains[arc.chain].ids[arc.begin:arc.end + 1])
            variant = variants.setdefault(ids.tobytes(), {"ids": ids, "arcs": []})
            variant["arcs"].append(arc)
        if len(variants) < 2:
            continue
        unequal_groups += 1
        remaining = set(variants)
        found_shared_cluster = False
        largest_distance = 0.0
        while remaining:
            best_key = min(remaining, key=lambda candidate: (-len(variants[candidate]["ids"]), candidate))
            selected = variants[best_key]["ids"]
            selected_points = projected[selected]
            line = LineString(selected_points)
            distances = {}
            for candidate in remaining:
                other = LineString(projected[variants[candidate]["ids"]])
                lower_bound = float(np.max(np.abs(np.array(line.bounds) - other.bounds)))
                distances[candidate] = lower_bound if lower_bound > epsilon else float(
                    arc_distance_upper_bound(selected_points, projected[variants[candidate]["ids"]]))
            largest_distance = max(largest_distance, max(distances.values()))
            # A ring with just two common anchors has two complementary arcs
            # sharing the same endpoints. Cluster by geometry before selecting
            # owners; the exterior arc must not suppress a genuine shared arc.
            cluster = {candidate for candidate, distance in distances.items() if distance <= epsilon}
            remaining -= cluster
            cluster_arcs = [arc for candidate in cluster for arc in variants[candidate]["arcs"]]
            cluster_owners = sorted({arc.region for arc in cluster_arcs})
            if len(cluster) < 2 or len(cluster_owners) < 2:
                continue
            found_shared_cluster = True
            detail = {
                "endpoint_key": key,
                "endpoints": coordinates[[selected[0], selected[-1]]].tolist(),
                "files": [borders[owner].name for owner in cluster_owners],
                "variant_point_counts": sorted(len(variants[candidate]["ids"]) for candidate in cluster),
                "hausdorff_mercator_upper_bound": max(distances[candidate] for candidate in cluster),
            }
            if any(not borders[owner].valid for owner in cluster_owners):
                skipped_invalid += 1
                continue
            if any(borders[owner].name in preserved for owner in cluster_owners):
                skipped_preserved += 1
                continue
            group_id = len(accepted)
            accepted[group_id] = (selected, cluster_arcs)
            detail["group_id"] = group_id
            group_details.append(detail)
        if not found_shared_cluster:
            skipped_far.append({
                "endpoint_key": key,
                "files": [borders[owner].name for owner in owners],
                "variant_point_counts": sorted(len(value["ids"]) for value in variants.values()),
                "endpoints": coordinates[[selected[0], selected[-1]]].tolist(),
                "distance_bound_mercator": largest_distance,
            })
    del candidates, projected
    LOG.info("Accepted %s unequal groups; %s exceed tolerance; %s touch invalid files; %s touch preserved files",
             len(accepted), len(skipped_far), skipped_invalid, skipped_preserved)
    blocked = set()
    passes = 0
    reverted_files = set()
    while True:
        passes += 1
        substitutions = {}
        per_file_groups = defaultdict(set)
        for key, (selected, arcs) in accepted.items():
            if key in blocked:
                continue
            for arc in arcs:
                original = chains[arc.chain].ids[arc.begin:arc.end + 1]
                replacement = selected[::-1] if original[0] > original[-1] else selected
                if np.array_equal(clean_ids(original), replacement):
                    continue
                substitutions[(arc.chain, arc.begin)] = replacement
                per_file_groups[arc.region].add(key)
        output = {border.name: [ring.points for ring in border.rings] for border in borders}
        for chain_index, chain in enumerate(chains):
            if not any((chain_index, int(begin)) in substitutions for begin in chain.anchors[:-1]):
                continue
            rebuilt = []
            for begin, end in zip(chain.anchors[:-1], chain.anchors[1:]):
                begin, end = int(begin), int(end)
                ids = substitutions.get((chain_index, begin), chain.ids[begin:end + 1])
                rebuilt.append(ids[:-1])
            ids = np.concatenate(rebuilt)
            if chain.closed:
                ids = np.r_[ids, ids[0]]
            output[borders[chain.region].name][chain.index] = coordinates[ids]
        failures = []
        for region, groups in per_file_groups.items():
            border = borders[region]
            points = [project(ring_points) for ring_points in output[border.name]]
            if any(ring.valid and (not Polygon(p).is_valid or Polygon(p).is_empty)
                   for ring, p in zip(border.rings, points)):
                failures.append(region)
                continue
            geometry = border_geometry(border, points)
            if border.valid and (geometry is None or not geometry.is_valid or geometry.is_empty):
                failures.append(region)
        if not failures:
            break
        for region in failures:
            blocked.update(per_file_groups[region])
            reverted_files.add(borders[region].name)
        LOG.info("Validation pass %s blocked %s groups in %s invalid outputs", passes, len(blocked), len(failures))
    changed = [border.name for border in borders
               if any(not np.array_equal(ring.points, points)
                      for ring, points in zip(border.rings, output[border.name]))]
    report = {
        "epsilon_mercator": epsilon,
        "algorithm": "Reuse denser unequal arcs between exact common vertices",
        "hausdorff_method": "Continuous upper bound from normalized arc-length coupling",
        "rejection_bounds": "Bounding-box lower bounds or normalized arc-length coupling upper bounds",
        "files": len(borders), "distinct_vertices": len(coordinates),
        "shared_vertices": int(shared.sum()), "unequal_endpoint_groups": unequal_groups,
        "normalized_endpoint_groups": len(accepted) - len(blocked),
        "candidate_within_tolerance": len(accepted), "groups_touching_invalid_files": skipped_invalid,
        "groups_touching_preserved_files": skipped_preserved, "preserved_files": sorted(preserved),
        "groups_exceeding_tolerance": len(skipped_far), "blocked_groups": len(blocked),
        "changed_files": changed, "validation_passes": passes,
        "invalid_source_files": invalid_files, "invalid_source_rings": invalid_rings,
        "reverted_files": sorted(reverted_files),
        "input_coordinate_records": int(sizes.sum()),
        "output_coordinate_records": sum(len(points) for rings in output.values() for points in rings),
        "normalized_groups": [d for d in group_details if d["group_id"] not in blocked],
        "largest_unresolved_groups": sorted(skipped_far, key=lambda d: -d["distance_bound_mercator"])[:100],
    }
    return output, report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--epsilon", type=float, default=DEFAULT_EPSILON)
    parser.add_argument("--preserve-file", action="append", default=[], metavar="NAME",
                        help="Lock all replacements involving this file")
    args = parser.parse_args(argv)
    source = args.input.resolve()
    destination = args.output.resolve()
    if source == destination or source in destination.parents or destination in source.parents:
        parser.error("Input and output directories must be separate, without containing each other")
    if not np.isfinite(args.epsilon) or args.epsilon <= 0:
        parser.error("Epsilon must be finite and positive")
    if args.output.exists() and any(args.output.iterdir()):
        parser.error("Output must be empty")
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    paths = sorted(args.input.glob("*.poly"))
    if not paths:
        parser.error("No .poly files found")
    LOG.info("Reading %s border files", len(paths))
    borders = [read_poly(path) for path in paths]
    output, report = normalize(borders, args.epsilon, args.preserve_file)
    args.output.mkdir(parents=True, exist_ok=True)
    changed = set(report["changed_files"])
    for border in borders:
        if border.name in changed:
            write_poly(args.output / border.name, border, output[border.name])
        else:
            shutil.copyfile(border.source, args.output / border.name)
    (args.output / "normalization-report.json").write_text(json.dumps(report, indent=2) + "\n")
    LOG.info("Wrote %s changed files to %s", len(changed), args.output)


if __name__ == "__main__":
    main()
