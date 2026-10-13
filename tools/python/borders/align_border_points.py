#!/usr/bin/env python3
"""Align unmatched border edges with nearby existing source coordinates.

Insert exact coordinates already shared by regions, or used by one donor
whose vertices include both target edge endpoints. Points must be interior
and within the current Mercator epsilon; coordinates are never averaged.
Ambiguous edges and preserved donors/targets are excluded. Valid targets
must remain valid. Explicitly allowed invalid files additionally require
no new local segment intersections and the same existing validity reason.
Historical edge exclusions that no longer match are recorded as no-ops.
"""

import argparse
from collections import defaultdict
import json
import logging
from pathlib import Path
import shutil

import numpy as np
from shapely import STRtree, linestrings, points as make_points
from shapely.geometry import LineString, Polygon
from shapely.validation import explain_validity

from maintain_borders import DEFAULT_EPSILON, read_poly, project, prepare_validity, border_geometry, write_poly

LOG = logging.getLogger(__name__)


def intersecting_replacement_pairs(replacements):
    """Yield each intersecting replacement pair once in edge order."""
    positions = sorted(replacements)
    geometries = [replacements[position] for position in positions]
    tree = STRtree(geometries)
    for at, geometry in enumerate(geometries):
        for other in sorted(tree.query(geometry, predicate="intersects")):
            if other > at:
                yield positions[at], positions[int(other)]


def exclusion_records(excluded_edges):
    """Canonicalize exclusions without relying on mutable ring/edge indices."""
    records = {}
    for entry in excluded_edges:
        if not isinstance(entry, dict):
            raise ValueError("Each excluded edge must be a file/endpoints/reason object")
        name = entry.get("file")
        if not isinstance(name, str) or not name:
            raise ValueError("Each excluded edge requires a file name")
        name = name if name.endswith(".poly") else name + ".poly"
        endpoints = np.asarray(entry.get("endpoints"), dtype=float)
        if endpoints.shape != (2, 2) or not np.isfinite(endpoints).all():
            raise ValueError(f"{name}: excluded edge requires two finite longitude/latitude endpoints")
        first, last = sorted(map(tuple, endpoints))
        if first == last:
            raise ValueError(f"{name}: excluded edge endpoints must differ")
        records[(name, first, last)] = {
            "file": name, "endpoints": [list(first), list(last)], "reason": entry.get("reason", ""),
        }
    return list(records.values())


def align_points(borders, epsilon=DEFAULT_EPSILON, preserve_files=(), allow_invalid_files=(), excluded_edges=()):
    """Return aligned coordinates and a report without writing files."""
    if not np.isfinite(epsilon) or epsilon <= 0:
        raise ValueError("Epsilon must be finite and positive")
    if not borders:
        raise ValueError("No borders provided")
    if len(borders) >= 2**16:
        raise ValueError("More than 65535 border files are not supported")
    preserved = {name if name.endswith(".poly") else name + ".poly" for name in preserve_files}
    allowed_invalid = {name if name.endswith(".poly") else name + ".poly" for name in allow_invalid_files}
    exclusions = exclusion_records(excluded_edges)
    unknown = (preserved | allowed_invalid | {entry["file"] for entry in exclusions}) - {border.name for border in borders}
    if unknown:
        raise ValueError(f"Unknown border files: {', '.join(sorted(unknown))}")
    unsupported = [border.name for border in borders if border.name in allowed_invalid and len(border.rings) != 1]
    if unsupported:
        raise ValueError(f"Explicit invalid-file alignment requires one ring: {', '.join(sorted(unsupported))}")
    invalid_rings, invalid_files = prepare_validity(borders)
    rings = [ring for border in borders for ring in border.rings]
    sizes = np.array([len(ring.points) for ring in rings])
    LOG.info("Interning %s coordinates", int(sizes.sum()))
    coordinates, inverse = np.unique(np.concatenate([ring.points for ring in rings]), axis=0, return_inverse=True)
    if len(coordinates) >= 2**32:
        raise ValueError("More than 2^32 distinct vertices are not supported")
    inverse = inverse.astype(np.uint32)
    minimum = np.full(len(coordinates), len(borders), dtype=np.uint16)
    maximum = np.zeros(len(coordinates), dtype=np.uint16)
    excluded_donors = np.zeros(len(coordinates), dtype=bool)
    ring_regions = []
    ring_indices = []
    closed = []
    region_nodes = []
    region_ring_nodes = []
    offset = 0
    for region, border in enumerate(borders):
        nodes = []
        for index, ring in enumerate(border.rings):
            size = len(ring.points)
            ids = inverse[offset:offset + size]
            offset += size
            np.minimum.at(minimum, ids, region)
            np.maximum.at(maximum, ids, region)
            if border.name in preserved:
                excluded_donors[ids] = True
            is_closed = bool(np.array_equal(ring.points[0], ring.points[-1]))
            ring.ids = ids[:-1] if is_closed else ids
            ring_regions.append(region)
            ring_indices.append(index)
            closed.append(is_closed)
            nodes.append(ids)
        region_nodes.append(np.unique(np.concatenate(nodes)))
        region_ring_nodes.append([region_nodes[-1]] if len(nodes) == 1 else [np.unique(ids) for ids in nodes])
    shared = minimum != maximum
    single_owners = minimum
    query_ids = np.flatnonzero(~excluded_donors).astype(np.uint32)
    del maximum, excluded_donors
    sizes = np.array([len(ring.ids) for ring in rings])
    starts = np.r_[0, np.cumsum(sizes)]
    keys = np.empty(int(sizes.sum()), dtype=np.uint64)
    for ring, begin, end in zip(rings, starts[:-1], starts[1:]):
        following = np.roll(ring.ids, -1)
        keys[begin:end] = (np.minimum(ring.ids, following).astype(np.uint64) << np.uint64(32)) | np.maximum(ring.ids, following)
    LOG.info("Filtering one-occurrence unmatched edges from %s edges", len(keys))
    unique_keys, first_indices, counts = np.unique(keys, return_index=True, return_counts=True)
    # Single occurrences are a conservative subset of one-owner edges. Repeated
    # edges, including repetitions inside one file, cannot be insertion targets.
    selected = counts == 1
    selected &= (unique_keys >> np.uint64(32)) != (unique_keys & np.uint64(0xffffffff))
    edge_ids = first_indices[selected]
    edge_keys = unique_keys[selected]
    del unique_keys, first_indices, counts, keys
    edge_rings = np.searchsorted(starts, edge_ids, side="right") - 1
    ring_regions = np.array(ring_regions)
    target_valid = np.array([(border.valid or border.name in allowed_invalid) and border.name not in preserved
                             for border in borders])
    selected = target_valid[ring_regions[edge_rings]]
    edge_ids = edge_ids[selected]
    edge_keys = edge_keys[selected]
    edge_rings = edge_rings[selected]
    owners = ring_regions[edge_rings]
    excluded_targets = np.zeros(len(edge_ids), dtype=bool)
    matched_exclusions = []
    unmatched_exclusions = []
    file_regions = {border.name: region for region, border in enumerate(borders)}
    for entry in exclusions:
        endpoint_ids = []
        for lon, lat in entry["endpoints"]:
            begin = int(np.searchsorted(coordinates[:, 0], lon, side="left"))
            end = int(np.searchsorted(coordinates[:, 0], lon, side="right"))
            position = begin + int(np.searchsorted(coordinates[begin:end, 1], lat))
            if position >= end or not np.array_equal(coordinates[position], (lon, lat)):
                break
            endpoint_ids.append(position)
        if len(endpoint_ids) != 2:
            unmatched_exclusions.append(entry)
            continue
        first_id, last_id = sorted(endpoint_ids)
        source = borders[file_regions[entry["file"]]]
        if not any(np.any(((ring.ids == first_id) & (np.roll(ring.ids, -1) == last_id))
                          | ((ring.ids == last_id) & (np.roll(ring.ids, -1) == first_id))) for ring in source.rings):
            unmatched_exclusions.append(entry)
            continue
        key = (np.uint64(first_id) << np.uint64(32)) | np.uint64(last_id)
        selected = (edge_keys == key) & (owners == file_regions[entry["file"]])
        excluded_targets |= selected
        if selected.any():
            matched_exclusions.append(entry)
        else:
            unmatched_exclusions.append(entry)
    first = (edge_keys >> np.uint64(32)).astype(np.uint32)
    last = (edge_keys & np.uint64(0xffffffff)).astype(np.uint32)
    projected = project(coordinates)
    segments = linestrings(np.stack((projected[first], projected[last]), axis=1))
    tree = STRtree(segments)
    LOG.info("Querying %s existing coordinates against %s eligible unmatched edges", len(query_ids), len(edge_ids))
    candidate_points = []
    candidate_edges = []
    counts = defaultdict(int)
    for begin in range(0, len(query_ids), 100000):
        batch = query_ids[begin:begin + 100000]
        query, edge = tree.query(make_points(projected[batch]), predicate="dwithin", distance=epsilon)
        ids = batch[query]
        counts["spatial_matches"] += len(ids)
        endpoint = (ids == first[edge]) | (ids == last[edge])
        counts["exact_endpoint_matches"] += int(endpoint.sum())
        keep = ~endpoint
        ids, edge = ids[keep], edge[keep]
        near_endpoint = ((np.linalg.norm(projected[ids] - projected[first[edge]], axis=1) <= epsilon)
                         | (np.linalg.norm(projected[ids] - projected[last[edge]], axis=1) <= epsilon))
        counts["near_endpoint_matches"] += int(near_endpoint.sum())
        keep = ~near_endpoint
        ids, edge = ids[keep], edge[keep]
        target_regions = owners[edge]
        present = np.zeros(len(ids), dtype=bool)
        for region in np.unique(target_regions):
            selected = np.flatnonzero(target_regions == region)
            nodes = region_nodes[region]
            positions = np.searchsorted(nodes, ids[selected])
            inside = positions < len(nodes)
            inside[inside] &= nodes[positions[inside]] == ids[selected][inside]
            present[selected] = inside
        counts["already_in_target_matches"] += int(present.sum())
        keep = ~present
        ids, edge, target_regions = ids[keep], edge[keep], target_regions[keep]
        # A point used by one donor alone needs stronger shared-path evidence:
        # both exact endpoints and the point must be in the same donor ring.
        justified = shared[ids].copy()
        for donor in np.unique(single_owners[ids[~justified]]):
            selected = np.flatnonzero((~justified) & (single_owners[ids] == donor))
            evidence = np.zeros(len(selected), dtype=bool)
            for nodes in region_ring_nodes[donor]:
                in_ring = np.ones(len(selected), dtype=bool)
                for vertices in (ids[selected], first[edge[selected]], last[edge[selected]]):
                    positions = np.searchsorted(nodes, vertices)
                    inside = positions < len(nodes)
                    inside[inside] &= nodes[positions[inside]] == vertices[inside]
                    in_ring &= inside
                evidence |= in_ring
            justified[selected] = evidence
        counts["single_owner_without_shared_path"] += int((~justified).sum())
        ids, edge, target_regions = ids[justified], edge[justified], target_regions[justified]
        pair_keys = (ids.astype(np.uint64) << np.uint64(16)) | target_regions.astype(np.uint64)
        _, inverse_pairs, pair_counts = np.unique(pair_keys, return_inverse=True, return_counts=True)
        ambiguous = pair_counts[inverse_pairs] > 1
        counts["ambiguous_target_matches"] += int(ambiguous.sum())
        excluded = excluded_targets[edge]
        counts["excluded_edge_matches"] += int((excluded & ~ambiguous).sum())
        # Keep excluded segments in the spatial query so they cannot resolve
        # an otherwise ambiguous match on another edge in the same target.
        keep = ~ambiguous & ~excluded
        if keep.any():
            candidate_points.append(ids[keep])
            candidate_edges.append(edge[keep])
    ids = np.concatenate(candidate_points) if candidate_points else np.array([], dtype=np.uint32)
    edge = np.concatenate(candidate_edges) if candidate_edges else np.array([], dtype=np.int64)
    LOG.info("Feasible candidates: %s exact donor vertices into %s unmatched edges across %s files",
             len(ids), len(np.unique(edge)), len(np.unique(owners[edge])))
    LOG.info("Query rejection counts: %s", dict(counts))
    groups = {}
    group_bounds = {}
    order = np.argsort(edge, kind="stable")
    sorted_edges, sorted_ids = edge[order], ids[order]
    group_starts = np.r_[0, np.flatnonzero(sorted_edges[1:] != sorted_edges[:-1]) + 1] if len(edge) else []
    group_ends = np.r_[group_starts[1:], len(edge)] if len(edge) else []
    for begin, end in zip(group_starts, group_ends):
        index = int(sorted_edges[begin])
        nodes = np.unique(sorted_ids[begin:end])
        ring = rings[edge_rings[index]]
        local = int(edge_ids[index] - starts[edge_rings[index]])
        a = projected[ring.ids[local]]
        b = projected[ring.ids[(local + 1) % len(ring.ids)]]
        parameters = ((projected[nodes] - a) @ (b - a)) / np.dot(b - a, b - a)
        interior = (parameters > 0) & (parameters < 1)
        nodes, parameters = nodes[interior], parameters[interior]
        if len(nodes):
            groups[int(index)] = nodes[np.argsort(parameters, kind="stable")]
            group_bounds[int(index)] = float(np.max(np.linalg.norm(
                projected[nodes] - a - parameters[:, None] * (b - a), axis=1)))
    per_ring = defaultdict(dict)
    for index, nodes in groups.items():
        per_ring[int(edge_rings[index])][int(edge_ids[index] - starts[edge_rings[index]])] = (index, nodes)
    blocked = set()
    local_failures = []
    invalid_source_reasons = {}
    for region, border in enumerate(borders):
        if border.valid or border.name not in allowed_invalid:
            continue
        geometry = border_geometry(border, [project(ring.points) for ring in border.rings])
        invalid_source_reasons[border.name] = explain_validity(geometry) if geometry is not None else "Missing geometry"
    # Explicitly allowed invalid files need a local proof: every replacement
    # intersection with an unchanged segment must already exist on the old
    # edge. Distant preexisting invalidities remain outside the edited arc.
    for ring_index, insertions in per_ring.items():
        border = borders[ring_regions[ring_index]]
        if border.valid:
            continue
        ring = rings[ring_index]
        ring_points = projected[ring.ids]
        original_segments = linestrings(np.stack((ring_points, np.roll(ring_points, -1, axis=0)), axis=1))
        ring_tree = STRtree(original_segments)
        replacements = {}
        for position, (index, nodes) in insertions.items():
            endpoints = ring.ids[[position, (position + 1) % len(ring.ids)]]
            replacement = LineString(projected[np.r_[endpoints[:1], nodes, endpoints[1:]]])
            replacements[position] = replacement
            original = original_segments[position]
            for other in ring_tree.query(replacement, predicate="intersects"):
                # Check every other original edge, including a candidate that
                # may later be rolled back. This proves safety for any subset.
                if int(other) == position:
                    continue
                old_intersection = original.intersection(original_segments[other])
                new_intersection = replacement.intersection(original_segments[other])
                if not new_intersection.difference(old_intersection).is_empty:
                    blocked.add(index)
                    local_failures.append({"file": border.name, "ring": ring.name,
                                           "edge": position, "intersected_edge": int(other)})
                    break
        for position, other in intersecting_replacement_pairs(replacements):
            old_intersection = original_segments[position].intersection(original_segments[other])
            new_intersection = replacements[position].intersection(replacements[other])
            if not new_intersection.difference(old_intersection).is_empty:
                blocked.update((insertions[position][0], insertions[other][0]))
                local_failures.append({"file": border.name, "ring": ring.name,
                                       "edge": position, "intersected_modified_edge": other})
    passes = 0
    reverted_files = set()
    while True:
        passes += 1
        output = {border.name: [ring.points for ring in border.rings] for border in borders}
        modified_regions = defaultdict(set)
        for ring_index, insertions in per_ring.items():
            ring = rings[ring_index]
            active = {position: (index, nodes) for position, (index, nodes) in insertions.items() if index not in blocked}
            if not active:
                continue
            rebuilt = []
            begin = 0
            for position, (index, nodes) in sorted(active.items()):
                rebuilt.append(ring.ids[begin:position + 1])
                rebuilt.append(nodes)
                begin = position + 1
                modified_regions[int(ring_regions[ring_index])].add(index)
            rebuilt.append(ring.ids[begin:])
            selected = np.concatenate(rebuilt)
            if closed[ring_index]:
                selected = np.r_[selected, selected[0]]
            output[borders[ring_regions[ring_index]].name][ring_indices[ring_index]] = coordinates[selected]
        failures = []
        for region in modified_regions:
            border = borders[region]
            points = [project(ring_points) for ring_points in output[border.name]]
            if any(ring.valid and (not Polygon(p).is_valid or Polygon(p).is_empty)
                   for ring, p in zip(border.rings, points)):
                failures.append(region)
                continue
            geometry = border_geometry(border, points)
            if border.valid and (geometry is None or not geometry.is_valid or geometry.is_empty):
                failures.append(region)
            elif not border.valid:
                reason = explain_validity(geometry) if geometry is not None else "Missing geometry"
                if reason != invalid_source_reasons[border.name]:
                    failures.append(region)
        if not failures:
            break
        for region in failures:
            blocked.update(modified_regions[region])
            reverted_files.add(borders[region].name)
        LOG.info("Validation pass %s blocked %s groups from %s invalid output files", passes, len(blocked), len(failures))
    changed = [border.name for border in borders if any(not np.array_equal(ring.points, p)
               for ring, p in zip(border.rings, output[border.name]))]
    details = []
    for index, nodes in groups.items():
        if index in blocked:
            continue
        details.append({"file": borders[owners[index]].name,
                        "ring": rings[edge_rings[index]].name,
                        "endpoints": coordinates[[first[index], last[index]]].tolist(),
                        "inserted_points": coordinates[nodes].tolist(),
                        "single_owner_points": int((~shared[nodes]).sum()),
                        "distance_bound_mercator": group_bounds[index]})
    report = {"epsilon_mercator": epsilon, "files": len(borders),
              "coordinates_queried": len(query_ids), "shared_coordinates": int(shared.sum()),
              "eligible_target_unmatched_edges": len(edge_ids),
              "query_counts": dict(counts), "candidate_insertions": len(ids),
              "candidate_edges": len(groups), "inserted_vertices": sum(len(nodes) for index, nodes in groups.items() if index not in blocked),
              "inserted_single_owner_vertices": sum(int((~shared[nodes]).sum()) for index, nodes in groups.items() if index not in blocked),
              "max_inserted_distance_mercator": max((bound for index, bound in group_bounds.items() if index not in blocked), default=0),
              "distance_bound": "Continuous Hausdorff bound for insertion ordered along each original segment",
              "inserted_edge_groups": len(groups) - len(blocked), "blocked_edge_groups": len(blocked),
              "validation_passes": passes, "reverted_files": sorted(reverted_files),
              "changed_files": changed, "invalid_source_files": invalid_files,
              "invalid_source_rings": invalid_rings, "preserved_files": sorted(preserved),
              "allowed_invalid_files": sorted(allowed_invalid), "invalid_source_reasons": invalid_source_reasons,
              "excluded_edges": exclusions, "matched_excluded_edges": matched_exclusions,
              "unmatched_excluded_edges": unmatched_exclusions,
              "local_intersection_rejections": local_failures, "insertions": details}
    return output, report


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--epsilon", type=float, default=DEFAULT_EPSILON)
    parser.add_argument("--preserve-file", action="append", default=[])
    parser.add_argument("--allow-invalid-file", action="append", default=[])
    parser.add_argument("--exclude-edges", type=Path, help="JSON array of file/endpoints/reason edge exclusions")
    args = parser.parse_args(argv)
    source = args.input.resolve()
    destination = args.output.resolve()
    if source == destination or source in destination.parents or destination in source.parents:
        parser.error("Input and output directories must be separate, without containing each other")
    if not np.isfinite(args.epsilon) or args.epsilon <= 0:
        parser.error("Epsilon must be finite and positive")
    if args.output.exists() and any(args.output.iterdir()):
        parser.error("Output directory must be empty or absent")
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    borders = [read_poly(path) for path in sorted(args.input.glob("*.poly"))]
    if not borders:
        parser.error("No .poly files found")
    excluded_edges = json.loads(args.exclude_edges.read_text()) if args.exclude_edges else ()
    output, report = align_points(borders, args.epsilon, args.preserve_file, args.allow_invalid_file, excluded_edges)
    args.output.mkdir(parents=True, exist_ok=True)
    changed = set(report["changed_files"])
    for border in borders:
        if border.name in changed:
            write_poly(args.output / border.name, border, output[border.name])
        else:
            shutil.copyfile(border.source, args.output / border.name)
    (args.output / "junction-report.json").write_text(json.dumps(report, indent=2) + "\n")
    LOG.info("Staged %s inserted vertices in %s changed files", report["inserted_vertices"], len(changed))


if __name__ == "__main__":
    main()
