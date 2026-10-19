"""Shared source measurements and strict baseline comparisons for border audits.

Coordinates are projected Mercator. Measured coverage never replaces source
records; inherited invalid contacts are still checked independently of area.
"""

from dataclasses import dataclass, field
import hashlib
import logging
import math

import numpy as np
from shapely import STRtree, prepare
from shapely.geometry import GeometryCollection, LineString, Point, Polygon
from shapely.ops import unary_union

from border_geometry import geometry_parts, measured_geometry, measurement_difference, ring_coverage, union_coverage
from maintain_borders import border_geometry, prepare_validity, project, read_poly


LOG = logging.getLogger(__name__)
# Two outer-path scale-10 tolerances, matching GetEpsilonForSimplify(10).
# This is an inspection limit for inherited contact movement, not an area tolerance.
PACKED_CONTACT_SHIFT = 2 * 360 * 1.3 / (256 << 10)


def area_m2(geometry):
    if geometry.is_empty or not geometry.area:
        return 0.0
    latitude = math.atan(math.sinh(math.radians(geometry.centroid.y)))
    return geometry.area * 111320**2 * math.cos(latitude)**2


def geographic_bounds(geometry):
    left, bottom, right, top = geometry.bounds
    return (left, math.degrees(math.atan(math.sinh(math.radians(bottom)))),
            right, math.degrees(math.atan(math.sinh(math.radians(top)))))


def geographic_point(geometry):
    point = geometry.representative_point()
    return Point(point.x, math.degrees(math.atan(math.sinh(math.radians(point.y))))).wkt


def load(directory):
    borders = [read_poly(path) for path in sorted(directory.glob("*.poly"))]
    if not borders:
        raise ValueError(f"No .poly files in {directory}")
    invalid_rings, invalid_files = prepare_validity(borders)
    shapes = []
    for border in borders:
        points = [project(ring.points) for ring in border.rings]
        geometry = border_geometry(border, points)
        shapes.append(measured_geometry(border, points, geometry))
    duplicate_records = 0
    unclosed = []
    for border in borders:
        for ring in border.rings:
            duplicate_records += int(np.count_nonzero(np.all(ring.points[1:] == ring.points[:-1], axis=1)))
            if not np.array_equal(ring.points[0], ring.points[-1]):
                unclosed.append({"file": border.name, "ring": ring.name})
    return borders, shapes, {
        "coordinate_system": "Degrees-scaled spherical Mercator, matching generator LoadBorders",
        "files": len(borders),
        "coordinate_records": sum(len(ring.points) for border in borders for ring in border.rings),
        "invalid_rings": invalid_rings,
        "invalid_files": invalid_files,
        "consecutive_duplicate_records": duplicate_records,
        "unclosed_rings": unclosed,
    }


def uncovered_loss(old, new, shapes, tree):
    """Find former coverage that no candidate region retains, including exterior gaps."""
    if old.equals_exact(new, 0):
        return Polygon()
    lost = measurement_difference(old, new)
    if not lost.area:
        return Polygon()
    for index in tree.query(lost):
        occupant = shapes[int(index)]
        # Near-collinear overlay fragments can disagree with prepared predicates
        # in older GEOS. Subtract every bounding-box candidate without shortcuts.
        lost = measurement_difference(lost, occupant)
        if not lost.area:
            return Polygon()
    return lost


@dataclass
class Dataset:
    names: list
    shapes: list
    invalid_rings: list
    invalid_files: list
    metadata: dict
    contacts: dict
    file_contacts: dict = field(default_factory=dict)
    contact_kinds: dict = field(default_factory=dict)


def ring_contacts(points):
    """Exact nonadjacent edge contacts, including inherited invalid ring defects."""
    return unary_union(list(ring_contact_kinds(points).values()))


def ring_contact_kinds(points):
    """Separate proper crossings, point touches and retraced segments."""
    if np.array_equal(points[0], points[-1]):
        points = points[:-1]
    edges = [LineString((point, points[(i + 1) % len(points)])) for i, point in enumerate(points)]
    tree = STRtree(edges)
    contacts = {"proper_crossings": [], "point_touches": [], "backtracks": []}
    for i, edge in enumerate(edges):
        if not edge.length:
            continue
        for index in tree.query(edge, predicate="intersects"):
            j = int(index)
            if j <= i or not edges[j].length:
                continue
            intersection = edge.intersection(edges[j])
            adjacent = j == i + 1 or (i == 0 and j == len(edges) - 1)
            for part in geometry_parts(intersection):
                if adjacent and not part.length:
                    continue
                kind = "backtracks" if part.geom_type == "LineString" else (
                    "proper_crossings" if edge.crosses(edges[j]) else "point_touches")
                contacts[kind].append(part)
    kinds = {kind: unary_union(parts) for kind, parts in contacts.items()}
    kinds["point_touches"] = measurement_difference(
        kinds["point_touches"], unary_union([kinds["backtracks"], kinds["proper_crossings"]]))
    return kinds


def contact_count(geometry):
    """Count connected contacts, independent of retraced segment subdivision."""
    parts = list(geometry_parts(geometry))
    parent = list(range(len(parts)))

    def owner(index):
        while parent[index] != index:
            parent[index] = parent[parent[index]]
            index = parent[index]
        return index

    tree = STRtree(parts)
    for i, part in enumerate(parts):
        for value in tree.query(part, predicate="intersects"):
            j = int(value)
            if j > i:
                parent[owner(j)] = owner(i)
    return len({owner(index) for index in range(len(parts))})


def point_contacts_fit(current, baseline):
    """Match every current point to a distinct nearby inherited contact."""
    old = list(geometry_parts(baseline))
    choices = [[index for index, point in enumerate(old) if point.distance(current_point) <= PACKED_CONTACT_SHIFT]
               for current_point in geometry_parts(current)]
    matches = {}

    def match(index, visited):
        for owner in choices[index]:
            if owner in visited:
                continue
            visited.add(owner)
            if owner not in matches or match(matches[owner], visited):
                matches[owner] = index
                return True
        return False

    return all(match(index, set()) for index in range(len(choices)))


def new_ring_contacts(candidate, baseline, key):
    """Packed point contacts may shift locally; source and backtrack geometry may not."""
    kinds = candidate.contact_kinds.get(key)
    old_kinds = baseline.contact_kinds.get(key)
    old = baseline.contacts.get(key, GeometryCollection())
    if kinds is None or old_kinds is None:
        return measurement_difference(candidate.contacts[key], old), {}
    increased, added = {}, []
    for kind, contacts in kinds.items():
        if kind == "point_touches":
            # Repairing a retraced segment can leave its endpoints as touches.
            contacts = measurement_difference(contacts, old_kinds["backtracks"])
        before, after = contact_count(old_kinds[kind]), contact_count(contacts)
        if kind == "backtracks":
            extra = measurement_difference(contacts, old_kinds[kind])
        else:
            extra = contacts if after > before or not point_contacts_fit(contacts, old_kinds[kind]) else GeometryCollection()
        if not extra.is_empty:
            increased[kind] = {"baseline": before, "current": after}
            added.append(extra)
    return unary_union(added), increased


def cross_ring_contacts(rings):
    """Cross-ring defects, allowing internal seams between valid disjoint outers."""
    if len(rings) < 2:
        return {}
    lines, coverage = [], []
    for _, points, _, valid in rings:
        closed = points if np.array_equal(points[0], points[-1]) else np.vstack((points, points[0]))
        lines.append(LineString(closed))
        coverage.append(ring_coverage(points, valid))
    tree = STRtree(lines)
    contacts = {}
    for i, line in enumerate(lines):
        for value in tree.query(line):
            j = int(value)
            if j <= i:
                continue
            contact = line.intersection(lines[j])
            # An outer intentionally contains its hole's interior.
            if rings[i][2] == rings[j][2]:
                overlap = coverage[i].intersection(coverage[j])
                contact = contact.union(overlap)
                if not rings[i][2] and rings[i][3] and rings[j][3] and not overlap.area:
                    # A shared edge partitions coverage without a gap or overlap.
                    # Keep isolated point contacts and all invalid-ring defects.
                    contact = unary_union([part for part in geometry_parts(contact) if part.geom_type == "Point"])
            if not contact.is_empty:
                contacts[rings[i][0], rings[j][0]] = contact
    return contacts


def source_dataset(directory, loaded=None):
    borders, shapes, metadata = load(directory) if loaded is None else loaded
    contacts, file_contacts = {}, {}
    for border in borders:
        for ring in border.rings:
            if not ring.valid:
                key = border.name, ring.name
                contacts[key] = ring_contacts(project(ring.points))
        if not border.valid:
            for pair, contact in cross_ring_contacts([
                    (ring.name, project(ring.points), ring.hole, ring.valid) for ring in border.rings]).items():
                file_contacts[border.name, *pair] = contact
    metadata["sha256"] = {border.name: hashlib.sha256(border.source.read_bytes()).hexdigest() for border in borders}
    metadata["file_identity"] = {border.name: {"title": border.title, "rings": [ring.name for ring in border.rings]}
                                 for border in borders}
    return Dataset([border.name for border in borders], shapes, metadata["invalid_rings"],
                   metadata["invalid_files"], metadata, contacts, file_contacts)


def finding(geometry, **details):
    return {**details, "approx_m2": area_m2(geometry), "bounds": geographic_bounds(geometry),
            "point": geographic_point(geometry)}


def coverage_levels(shapes, tree, window):
    """Return local geometry covered by at least 1, 2, ... distinct region owners."""
    levels = []
    for value in tree.query(window, predicate="intersects"):
        piece = shapes[int(value)].intersection(window)
        if not piece.area:
            continue
        for index in range(len(levels) - 1, -1, -1):
            added = levels[index].intersection(piece)
            if not added.area:
                continue
            if index + 1 == len(levels):
                levels.append(added)
            else:
                levels[index + 1] = levels[index + 1].union(added)
        if levels:
            levels[0] = levels[0].union(piece)
        else:
            levels.append(piece)
    return levels


def introduced_ambiguity(window, candidate, current_tree, baseline, old_tree):
    current = coverage_levels(candidate.shapes, current_tree, window)
    old = coverage_levels(baseline.shapes, old_tree, window)
    # A third owner is a regression even where the baseline already had two.
    return unary_union([measurement_difference(level, old[index] if index < len(old) else Polygon())
                        for index, level in enumerate(current) if index >= 1])


def changed_third_owner(window, candidate, tree, unchanged, pair):
    """Conservatively retain multiplicity checks if another changed owner intersects."""
    return any(int(index) not in pair and candidate.names[int(index)] not in unchanged
               for index in tree.query(window, predicate="intersects"))


def compare(candidate, baseline, minimum_area, coverage_losses=None):
    old_invalid = {(item["file"], item["ring"]) for item in baseline.invalid_rings}
    new_invalid = [item for item in candidate.invalid_rings if (item["file"], item["ring"]) not in old_invalid]
    new_contacts = []
    for key, contacts in candidate.contacts.items():
        added, increased = new_ring_contacts(candidate, baseline, key)
        if not added.is_empty:
            new_contacts.append({"file": key[0], "ring": key[1], "bounds": geographic_bounds(added),
                                 "point": geographic_point(added), "increased_kinds": increased})
    new_file_contacts = []
    for key, contacts in candidate.file_contacts.items():
        added = measurement_difference(contacts, baseline.file_contacts.get(key, GeometryCollection()))
        if not added.is_empty:
            new_file_contacts.append({"file": key[0], "rings": list(key[1:]),
                                      "bounds": geographic_bounds(added), "point": geographic_point(added)})
    old_tree = STRtree(baseline.shapes)
    new_tree = STRtree(candidate.shapes)
    prepare(candidate.shapes)
    overlaps, losses = [], []
    old_by_name = dict(zip(baseline.names, baseline.shapes))
    unchanged = {name for name, shape in zip(candidate.names, candidate.shapes)
                 if name in old_by_name and shape.equals_exact(old_by_name[name], 0)}
    for i, shape in enumerate(candidate.shapes):
        for value in new_tree.query(shape, predicate="intersects"):
            j = int(value)
            if j <= i or (candidate.names[i] in unchanged and candidate.names[j] in unchanged):
                continue
            overlap = shape.intersection(candidate.shapes[j])
            if area_m2(overlap) <= minimum_area:
                continue
            first, second = candidate.names[i], candidate.names[j]
            pair_growth = Polygon()
            if first in old_by_name and second in old_by_name:
                # Established owners must not gain a new pair-specific overlap.
                pair_growth = union_coverage([measurement_difference(overlap, old_by_name[first]),
                                              measurement_difference(overlap, old_by_name[second])])
                # The old pair already covers this window twice. Unchanged extra
                # owners contribute the same multiplicity; removed ones reduce it.
                if pair_growth.is_empty and not changed_third_owner(overlap, candidate, new_tree, unchanged, (i, j)):
                    continue
            introduced = union_coverage([introduced_ambiguity(overlap, candidate, new_tree, baseline, old_tree),
                                         pair_growth])
            if area_m2(introduced) > minimum_area:
                overlaps.append(finding(introduced, owners=[candidate.names[i], candidate.names[j]]))
        if i % 100 == 0:
            LOG.info("Compared overlap for %s/%s regions", i, len(candidate.names))
    current_by_name = dict(zip(candidate.names, candidate.shapes))
    for index, (name, old) in enumerate(zip(baseline.names, baseline.shapes)):
        lost = uncovered_loss(old, current_by_name.get(name, Polygon()), candidate.shapes, new_tree)
        if coverage_losses is not None and not lost.is_empty:
            coverage_losses.append((name, lost))
        if area_m2(lost) > minimum_area:
            losses.append(finding(lost, file=name))
        if index % 100 == 0:
            LOG.info("Compared retained coverage for %s/%s baseline regions", index, len(baseline.names))
    old_identity = baseline.metadata.get("file_identity", {})
    changed_metadata = [name for name, identity in candidate.metadata.get("file_identity", {}).items()
                        if name in old_identity and identity != old_identity[name]]
    report = {"changed_metadata": sorted(changed_metadata), "added_names": sorted(set(candidate.names) - set(baseline.names)),
              "removed_names": sorted(set(baseline.names) - set(candidate.names)),
              "invalid_rings": candidate.invalid_rings, "inherited_invalid_rings": [
                  item for item in candidate.invalid_rings if (item["file"], item["ring"]) in old_invalid],
              "new_invalid_rings": new_invalid,
              "new_invalid_files": sorted(set(candidate.invalid_files) - set(baseline.invalid_files)),
              "new_invalid_ring_contacts": new_contacts,
              "new_invalid_file_contacts": new_file_contacts,
              "introduced_overlap_parts": sorted(overlaps, key=lambda item: -item["approx_m2"]),
              "introduced_uncovered_parts": sorted(losses, key=lambda item: -item["approx_m2"])}
    report["regressions"] = any(report[key] for key in ("new_invalid_rings", "new_invalid_files",
                                                       "new_invalid_ring_contacts", "introduced_overlap_parts",
                                                       "new_invalid_file_contacts",
                                                       "introduced_uncovered_parts"))
    return report
