#!/usr/bin/env python3
"""Simplify .poly boundaries once per shared arc, using Douglas-Peucker.

Requires Python 3.10+, NumPy and Shapely. Coordinates are matched exactly;
existing gaps are not snapped or repaired. Unshared edges of touching regions
are preserved, so unmatched borders cannot be simplified independently.
Epsilon is in Organic Maps' Mercator
units (longitude degrees, with the projected vertical coordinate in the same
units). The default 1e-5 corresponds to about 1.11 m at the equator.

Usage: python3 maintain_borders.py data/borders /tmp/simplified-borders

Output must be separate from input. Invalid source rings and explicitly
preserved files lock their arcs in every neighboring region. Newly invalid
geometries also lock their arcs, then all affected regions are reconstructed.
Unchanged files are copied byte for byte; a JSON report records the result.
"""

import argparse
from dataclasses import dataclass, field
import json
import logging
from pathlib import Path
import shutil

import numpy as np
from shapely.geometry import MultiPolygon, Polygon
from shapely.validation import explain_validity


LOG = logging.getLogger(__name__)
DEFAULT_EPSILON = 1e-5


@dataclass
class Ring:
    name: str
    points: np.ndarray
    ids: np.ndarray | None = None
    junctions: np.ndarray | None = None
    chain: np.ndarray | None = None
    valid: bool = True

    @property
    def hole(self):
        return self.name.startswith("!")


@dataclass
class Border:
    name: str
    title: str
    rings: list[Ring]
    source: Path | None = None
    valid: bool = True
    hole_owners: dict = field(default_factory=dict)


def read_poly(path):
    """Read title and ring names without treating a hole as an outer ring."""
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines:
        raise ValueError(f"{path}: empty .poly file")
    title = lines[0]
    index = 1
    rings = []
    while index < len(lines):
        name = lines[index].strip()
        index += 1
        if not name:
            continue
        if name == "END":
            if any(line.strip() for line in lines[index:]):
                raise ValueError(f"{path}: content after final END")
            if not rings:
                raise ValueError(f"{path}: no rings")
            return Border(path.name, title, rings, path)
        begin = index
        while index < len(lines) and lines[index].strip() != "END":
            index += 1
        if index == len(lines):
            raise ValueError(f"{path}: unclosed ring {name}")
        # NumPy parses large coordinate blocks without millions of Python tuples.
        rows = [line for line in lines[begin:index] if line.strip()]
        if len(rows) < 3:
            raise ValueError(f"{path}: invalid coordinates in ring {name}")
        try:
            points = np.loadtxt(rows, dtype=np.float64, comments=None, ndmin=2)
        except ValueError as error:
            raise ValueError(f"{path}: invalid coordinates in ring {name}") from error
        if points.shape[1] != 2 or not np.isfinite(points).all():
            raise ValueError(f"{path}: invalid coordinates in ring {name}")
        if np.any(np.abs(points[:, 1]) > 90):
            raise ValueError(f"{path}: latitude outside [-90, 90] in ring {name}")
        rings.append(Ring(name, points))
        index += 1
    raise ValueError(f"{path}: missing final END")


def project(points):
    """Organic Maps' degrees-scaled spherical Mercator projection."""
    result = points.copy()
    sine = np.sin(np.radians(np.clip(result[:, 1], -86.0, 86.0)))
    result[:, 1] = np.clip(np.degrees(0.5 * np.log((1 + sine) / (1 - sine))), -180.0, 180.0)
    return result


def douglas_peucker(ids, coordinates, epsilon):
    """Return retained IDs, including both endpoints; distance is to a segment."""
    if len(ids) <= 2:
        return ids
    points = coordinates[ids]
    keep = np.zeros(len(ids), dtype=bool)
    keep[0] = keep[-1] = True
    pending = [(0, len(ids) - 1)]
    squared_epsilon = epsilon * epsilon
    while pending:
        first, last = pending.pop()
        if last - first <= 1:
            continue
        direction = points[last] - points[first]
        offsets = points[first + 1:last] - points[first]
        length = np.dot(direction, direction)
        if length:
            parameters = np.clip(offsets @ direction / length, 0, 1)
            offsets -= parameters[:, None] * direction
        distances = np.einsum("ij,ij->i", offsets, offsets)
        index = int(np.argmax(distances))
        if distances[index] >= squared_epsilon:
            split = first + 1 + index
            keep[split] = True
            pending.extend(((split, last), (first, split)))
    return ids[keep]


def canonical_closed(ids):
    """Normalize cyclic rotation and direction, retaining the closing vertex."""
    ids = ids[:-1] if ids[0] == ids[-1] else ids
    candidates = np.flatnonzero(ids == ids.min())
    best = None
    for index in candidates:
        forward = np.concatenate((ids[index:], ids[:index]))
        backward = np.concatenate((forward[:1], forward[:0:-1]))
        differing = np.flatnonzero(forward != backward)
        if len(differing) and backward[differing[0]] < forward[differing[0]]:
            forward = backward
        if best is None:
            best = forward
        else:
            differing = np.flatnonzero(best != forward)
            if len(differing) and forward[differing[0]] < best[differing[0]]:
                best = forward
    return np.concatenate((best, best[:1]))


def canonical_arc(ids, cyclic=False):
    if cyclic:
        canonical = canonical_closed(ids)
        return b"C" + canonical.tobytes(), canonical, False
    reversed_arc = ids[0] > ids[-1]
    if ids[0] == ids[-1]:
        differing = np.flatnonzero(ids != ids[::-1])
        reversed_arc = bool(len(differing) and ids[::-1][differing[0]] < ids[differing[0]])
    canonical = ids[::-1] if reversed_arc else ids
    return b"A" + canonical.tobytes(), canonical, reversed_arc


def iter_arcs(ring):
    if not len(ring.junctions):
        yield ring.chain
        return
    for first, last in zip(ring.junctions[:-1], ring.junctions[1:]):
        yield ring.chain[first:last + 1]


def border_geometry(border, points):
    """Build geometry in the supplied coordinate plane."""
    outers = [index for index, ring in enumerate(border.rings) if not ring.hole]
    if not outers:
        return None
    polygons = []
    for index in outers:
        holes = [points[hole] for hole, owner in border.hole_owners.items() if owner == index]
        polygons.append(Polygon(points[index], holes))
    if any(ring.hole and index not in border.hole_owners for index, ring in enumerate(border.rings)):
        return None
    return MultiPolygon(polygons) if len(polygons) > 1 else polygons[0]


def prepare_validity(borders):
    invalid_rings = []
    invalid_files = []
    for border in borders:
        border.hole_owners.clear()
        points = [project(ring.points) for ring in border.rings]
        polygons = [Polygon(ring_points) for ring_points in points]
        for index, (ring, polygon) in enumerate(zip(border.rings, polygons)):
            ring.valid = polygon.is_valid and not polygon.is_empty
            if not ring.valid:
                invalid_rings.append({"file": border.name, "ring": ring.name, "reason": explain_validity(polygon)})
            if ring.hole:
                owners = [
                    owner for owner, outer in enumerate(polygons)
                    if not border.rings[owner].hole and outer.covers(polygon.representative_point())
                ]
                if owners:
                    border.hole_owners[index] = min(owners, key=lambda owner: polygons[owner].area)
        geometry = border_geometry(border, points)
        border.valid = geometry is not None and geometry.is_valid and not geometry.is_empty
        if not border.valid:
            invalid_files.append(border.name)
    return invalid_rings, invalid_files


def prepare_topology(borders, preserve_files=()):
    """Intern vertices and assign an integer label to each edge ownership set."""
    rings = [ring for border in borders for ring in border.rings]
    sizes = np.array([len(ring.points) for ring in rings], dtype=np.int64)
    coordinates, inverse = np.unique(np.concatenate([ring.points for ring in rings]), axis=0, return_inverse=True)
    if len(coordinates) >= 2**32:
        raise ValueError("More than 2^32 distinct vertices are not supported")
    inverse = inverse.astype(np.uint32)
    position = 0
    edge_count = 0
    for ring, size in zip(rings, sizes):
        ids = inverse[position:position + size]
        position += size
        ids = ids[np.r_[True, ids[1:] != ids[:-1]]]
        if len(ids) > 1 and ids[-1] == ids[0]:
            ids = ids[:-1]
        ring.ids = ids
        edge_count += len(ids)
    del inverse
    protected = np.zeros(len(coordinates), dtype=bool)
    for border in borders:
        for ring in border.rings:
            if not ring.valid or border.name in preserve_files:
                protected[ring.ids] = True

    # Packing uint32 endpoints into uint64 reduces sorting and indexing memory.
    edge_keys = np.empty(edge_count, dtype=np.uint64)
    region_ids = np.empty(edge_count, dtype=np.uint32)
    position = 0
    for region, border in enumerate(borders):
        for ring in border.rings:
            ids = ring.ids
            next_ids = np.roll(ids, -1)
            size = len(ids)
            edge_keys[position:position + size] = (
                np.minimum(ids, next_ids).astype(np.uint64) << np.uint64(32)
            ) | np.maximum(ids, next_ids)
            region_ids[position:position + size] = region
            position += size
    order = np.lexsort((region_ids, edge_keys))
    sorted_keys = edge_keys[order]
    sorted_regions = region_ids[order]
    starts = np.r_[0, np.flatnonzero(sorted_keys[1:] != sorted_keys[:-1]) + 1]
    ends = np.r_[starts[1:], edge_count]
    # Deduplicate (edge, region) records, then vectorize the common one/two-owner cases.
    unique_pairs = np.r_[True, (sorted_keys[1:] != sorted_keys[:-1]) | (sorted_regions[1:] != sorted_regions[:-1])]
    unique_keys = sorted_keys[unique_pairs]
    unique_regions = sorted_regions[unique_pairs]
    owner_starts = np.r_[0, np.flatnonzero(unique_keys[1:] != unique_keys[:-1]) + 1]
    owner_counts = np.diff(np.r_[owner_starts, len(unique_keys)])
    group_labels = unique_regions[owner_starts].copy()
    double_owners = np.flatnonzero(owner_counts == 2)
    double_starts = owner_starts[double_owners]
    pairs = (unique_regions[double_starts].astype(np.uint64) << np.uint64(32)) | unique_regions[double_starts + 1]
    pairs, pair_labels = np.unique(pairs, return_inverse=True)
    group_labels[double_owners] = pair_labels + len(borders)
    labels = {}
    next_label = len(borders) + len(pairs)
    for index in np.flatnonzero(owner_counts > 2):
        first = owner_starts[index]
        owners = tuple(unique_regions[first:first + owner_counts[index]].tolist())
        if owners not in labels:
            labels[owners] = next_label
            next_label += 1
        group_labels[index] = labels[owners]
    shared_edges = int(np.count_nonzero(owner_counts > 1))
    owner_labels = np.repeat(group_labels, ends - starts)
    edge_labels = np.empty(edge_count, dtype=np.uint32)
    edge_labels[order] = owner_labels
    distinct_keys = sorted_keys[starts]
    degree = np.bincount((distinct_keys >> np.uint64(32)).astype(np.int64), minlength=len(coordinates))
    degree += np.bincount((distinct_keys & np.uint64(0xffffffff)).astype(np.int64), minlength=len(coordinates))
    position = 0
    unmatched_edges = 0
    for ring in rings:
        size = len(ring.ids)
        sharing = edge_labels[position:position + size]
        position += size
        if np.any(sharing >= len(borders)) or np.any(degree[ring.ids] != 2):
            unshared = sharing < len(borders)
            unmatched_edges += int(np.count_nonzero(unshared))
            protected[ring.ids[unshared]] = True
            protected[np.roll(ring.ids, -1)[unshared]] = True
    position = 0
    junction_count = 0
    for ring in rings:
        size = len(ring.ids)
        sharing = edge_labels[position:position + size]
        position += size
        junctions = np.flatnonzero((sharing != np.roll(sharing, 1)) | (degree[ring.ids] != 2) | protected[ring.ids])
        junction_count += len(junctions)
        if len(junctions):
            start = junctions[0]
            rotated = np.concatenate((ring.ids[start:], ring.ids[:start]))
            ring.chain = np.concatenate((rotated, rotated[:1]))
            ring.junctions = np.r_[junctions - start, size]
        else:
            ring.chain = np.concatenate((ring.ids, ring.ids[:1]))
            ring.junctions = np.array([], dtype=np.int64)
    return coordinates, {
        "distinct_vertices": len(coordinates),
        "distinct_edges": len(starts),
        "shared_edges": shared_edges,
        "ownership_sets": len(pairs) + len(labels),
        "junction_occurrences": junction_count,
        "protected_vertices": int(np.count_nonzero(protected)),
        "protected_unshared_edge_occurrences": unmatched_edges,
    }


def simplify_borders(borders, epsilon=DEFAULT_EPSILON, preserve_files=()):
    """Return coordinates per file/ring and a report, without writing files."""
    if not np.isfinite(epsilon) or epsilon <= 0:
        raise ValueError("Epsilon must be finite and positive")
    preserved = {name if name.endswith(".poly") else name + ".poly" for name in preserve_files}
    unknown = preserved - {border.name for border in borders}
    if unknown:
        raise ValueError(f"Unknown preserved files: {', '.join(sorted(unknown))}")
    LOG.info("Checking source geometry validity")
    invalid_rings, invalid_files = prepare_validity(borders)
    LOG.info("Interning vertices and building shared-edge topology")
    coordinates, report = prepare_topology(borders, preserved)
    projected = project(coordinates)
    cache = {}
    blocked = set()
    for border in borders:
        for ring in border.rings:
            if not ring.valid or border.name in preserved:
                for arc in iter_arcs(ring):
                    if len(arc) > 2:
                        blocked.add(canonical_arc(arc, cyclic=not len(ring.junctions))[0])

    def retained_arc(arc, cyclic):
        if len(arc) <= 2:
            return arc
        key, canonical, reversed_arc = canonical_arc(arc, cyclic)
        if key in blocked:
            return arc
        if key not in cache:
            retained = douglas_peucker(canonical, projected, epsilon)
            if canonical[0] == canonical[-1] and len(np.unique(retained)) < 3:
                retained = canonical
            cache[key] = retained
        retained = cache[key]
        return retained[::-1] if reversed_arc else retained

    def reconstruct(ring):
        if not ring.valid:
            return ring.ids
        arcs = [retained_arc(arc, cyclic=not len(ring.junctions)) for arc in iter_arcs(ring)]
        ids = np.concatenate([arcs[0]] + [arc[1:] for arc in arcs[1:]])
        if ids[0] == ids[-1]:
            ids = ids[:-1]
        # If no vertices were removed, preserve the original order and formatting.
        return ring.ids if len(ids) == len(ring.ids) else ids

    reverted_rings = set()
    reverted_files = set()
    passes = 0
    while True:
        passes += 1
        LOG.info("Simplifying arcs and validating output (pass %s)", passes)
        results = {}
        newly_blocked = set()
        invalid_outputs = []
        for border in borders:
            rings = [reconstruct(ring) for ring in border.rings]
            points = [projected[ids] for ids in rings]
            invalid_ring_output = False
            for index, (ring, ids, ring_points) in enumerate(zip(border.rings, rings, points)):
                if ring.valid and (len(np.unique(ids)) < 3 or not Polygon(ring_points).is_valid):
                    invalid_ring_output = True
                    invalid_outputs.append(f"{border.name}/{ring.name}")
                    reverted_rings.add((border.name, ring.name))
                    for arc in iter_arcs(ring):
                        if len(arc) > 2:
                            newly_blocked.add(canonical_arc(arc, cyclic=not len(ring.junctions))[0])
            if border.valid:
                # A collapsed ring cannot construct a Polygon; lock its arcs and retry first.
                geometry = None if invalid_ring_output else border_geometry(border, points)
                if geometry is None or not geometry.is_valid or geometry.is_empty:
                    invalid_outputs.append(border.name)
                    reverted_files.add(border.name)
                    for ring in border.rings:
                        for arc in iter_arcs(ring):
                            if len(arc) > 2:
                                newly_blocked.add(canonical_arc(arc, cyclic=not len(ring.junctions))[0])
            results[border.name] = rings
        newly_blocked -= blocked
        if not newly_blocked:
            if invalid_outputs:
                raise ValueError(f"Invalid output despite preserved arcs: {', '.join(invalid_outputs)}")
            break
        blocked.update(newly_blocked)

    output = {}
    changed_files = []
    removed = 0
    for border in borders:
        points = []
        changed = False
        for ring, ids in zip(border.rings, results[border.name]):
            if np.array_equal(ids, ring.ids):
                points.append(ring.points)
            else:
                changed = True
                removed += len(ring.ids) - len(ids)
                selected = coordinates[ids]
                points.append(np.concatenate((selected, selected[:1])))
        output[border.name] = points
        if changed:
            changed_files.append(border.name)
    report.update({
        "epsilon_mercator": epsilon,
        "algorithm": "Douglas-Peucker on canonical shared arcs",
        "files": len(borders),
        "rings": sum(len(border.rings) for border in borders),
        "input_coordinate_records": sum(len(ring.points) for border in borders for ring in border.rings),
        "output_coordinate_records": sum(len(points) for rings in output.values() for points in rings),
        "removed_vertices": removed,
        "changed_files": changed_files,
        "cached_arcs": len(cache),
        "locked_arcs": len(blocked),
        "validation_passes": passes,
        "invalid_source_rings": invalid_rings,
        "invalid_source_files": invalid_files,
        "preserved_files": sorted(preserved),
        "reverted_rings": [{"file": name, "ring": ring} for name, ring in sorted(reverted_rings)],
        "reverted_files": sorted(reverted_files),
    })
    return output, report


def write_poly(path, border, points):
    with path.open("w", encoding="utf-8", newline="\n") as stream:
        stream.write(border.title + "\n")
        for ring, ring_points in zip(border.rings, points):
            stream.write(ring.name + "\n")
            # Round-trip precision keeps shared coordinates exactly identical.
            for lon, lat in ring_points:
                stream.write(f"\t{float(lon)!r}\t{float(lat)!r}\n")
            stream.write("END\n")
        stream.write("END\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input_dir", type=Path)
    parser.add_argument("output_dir", type=Path)
    parser.add_argument("--epsilon", type=float, default=DEFAULT_EPSILON, help="Mercator tolerance (default: 1e-5)")
    parser.add_argument("--preserve-file", action="append", default=[], metavar="NAME", help="Lock all arcs of this file")
    args = parser.parse_args()
    input_dir = args.input_dir.resolve()
    output_dir = args.output_dir.resolve()
    if input_dir == output_dir or input_dir in output_dir.parents or output_dir in input_dir.parents:
        parser.error("Input and output directories must be separate, without containing each other")
    if output_dir.exists() and any(output_dir.iterdir()):
        parser.error("Output directory must be empty or absent")
    paths = sorted(input_dir.glob("*.poly"))
    if not paths:
        parser.error("No .poly files found")
    logging.basicConfig(level=logging.INFO, format="%(asctime)s %(message)s")
    LOG.info("Reading %s border files", len(paths))
    borders = [read_poly(path) for path in paths]
    output, report = simplify_borders(borders, args.epsilon, args.preserve_file)
    output_dir.mkdir(parents=True, exist_ok=True)
    changed = set(report["changed_files"])
    for border in borders:
        path = output_dir / border.name
        if border.name in changed:
            write_poly(path, border, output[border.name])
        else:
            shutil.copyfile(border.source, path)
    (output_dir / "report.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    LOG.info("Removed %s vertices in %s files; report: %s", report["removed_vertices"], len(changed), output_dir / "report.json")


if __name__ == "__main__":
    main()
