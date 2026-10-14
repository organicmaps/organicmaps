#!/usr/bin/env python3
"""Read the default 30-bit packed border format used by CountryInfoReader.

Matches FilesContainer, CountryDef and LoadOuterPath in libs/coding.
CountryDef has unversioned legacy signed[-180,180] and wrapped
unsigned[-360,360] longitude bounds; callers must select the matching variant.
Coordinates remain on the stored Mercator grid throughout audits.
"""

from pathlib import Path
import struct

import numpy as np


COORD_BITS = 30
COORD_LIMIT = (1 << COORD_BITS) - 1


class Source:
    def __init__(self, data, start=0, end=None):
        self.data = data
        self.position = start
        self.end = len(data) if end is None else end

    def varuint(self):
        value = 0
        for shift in range(0, 70, 7):
            if self.position >= self.end:
                raise ValueError("Truncated packed border section")
            byte = self.data[self.position]
            self.position += 1
            value |= (byte & 127) << shift
            if byte < 128:
                return value
        raise ValueError("Invalid packed border varuint")

    def string(self):
        size = self.varuint()
        end = self.position + size
        if end > self.end:
            raise ValueError("Truncated packed border string")
        value = self.data[self.position:end].decode("utf-8")
        self.position = end
        return value

    def buffer(self):
        size = self.varuint()
        end = self.position + size
        if end > self.end:
            raise ValueError("Truncated packed geometry buffer")
        result = Source(self.data, self.position, end)
        self.position = end
        return result

    def finish(self):
        if self.position != self.end:
            raise ValueError("Unexpected bytes after packed border section")


def split_bits(value):
    def compact(part):
        part &= 0x5555555555555555
        part = (part | (part >> 1)) & 0x3333333333333333
        part = (part | (part >> 2)) & 0x0F0F0F0F0F0F0F0F
        part = (part | (part >> 4)) & 0x00FF00FF00FF00FF
        part = (part | (part >> 8)) & 0x0000FFFF0000FFFF
        return (part | (part >> 16)) & 0xFFFFFFFF
    return compact(value), compact(value >> 1)


def decode_delta(value, prediction):
    x, y = split_bits(value)
    point = (prediction[0] + ((x >> 1) ^ -(x & 1)), prediction[1] + ((y >> 1) ^ -(y & 1)))
    if not all(0 <= coordinate <= COORD_LIMIT for coordinate in point):
        raise ValueError("Packed geometry is outside the default 30-bit coordinate grid")
    return point


def predict_polyline(first, second):
    return tuple(min(COORD_LIMIT, max(0, (3 * first[i] - second[i]) // 2)) for i in (0, 1))


def mercator(points):
    return np.asarray(points, dtype=np.float64) * 360.0 / COORD_LIMIT - 180.0


def bound_point(value, bounds_format):
    if bounds_format == "legacy":
        value = (value >> 1) ^ -(value & 1)
        if value < 0:
            raise ValueError("Legacy CountryDef rectangle point is negative")
    x, y = split_bits(value)
    if x > COORD_LIMIT or y > COORD_LIMIT:
        raise ValueError("CountryDef bounds are outside the default 30-bit coordinate grid")
    longitude_limit = 180.0 if bounds_format == "legacy" else 360.0
    return -longitude_limit + x * (2 * longitude_limit) / COORD_LIMIT, -180.0 + y * 360.0 / COORD_LIMIT


def decode_path(source):
    points = []
    while source.position < source.end:
        prediction = (0, 0) if not points else points[0] if len(points) == 1 else predict_polyline(points[-1], points[-2])
        points.append(decode_delta(source.varuint(), prediction))
    return np.asarray(points, dtype=np.uint32).reshape(-1, 2)


class PackedBorders:
    def __init__(self, path, bounds_format="wrapped"):
        if bounds_format not in ("legacy", "wrapped"):
            raise ValueError("Unknown CountryDef bounds format")
        self.bounds_format = bounds_format
        self.path = Path(path)
        self.data = self.path.read_bytes()
        if len(self.data) < 8:
            raise ValueError("Packed borders container is incomplete")
        directory = struct.unpack_from("<Q", self.data)[0]
        if not 8 <= directory < len(self.data):
            raise ValueError("Packed borders container has no completed section directory")
        source = Source(self.data, directory)
        self.sections = {}
        for _ in range(source.varuint()):
            tag = source.string()
            offset, size = source.varuint(), source.varuint()
            if tag in self.sections or offset < 8 or offset + size > directory:
                raise ValueError("Invalid packed border section directory")
            self.sections[tag] = (offset, size)
        source.finish()
        source = self.section("info")
        self.names = []
        self.bounds = {}
        for _ in range(source.varuint()):
            name = source.string()
            if name in self.bounds:
                raise ValueError(f"Duplicate packed border name: {name}")
            bounds = (*bound_point(source.varuint(), bounds_format), *bound_point(source.varuint(), bounds_format))
            if bounds[0] > bounds[2] or bounds[1] > bounds[3]:
                raise ValueError(f"Reversed CountryDef bounds: {name}")
            self.names.append(name)
            self.bounds[name] = bounds
        source.finish()
        if not self.names:
            raise ValueError("Packed borders contain no regions")

    def section(self, tag):
        if tag not in self.sections:
            raise ValueError(f"Missing packed border section: {tag}")
        offset, size = self.sections[tag]
        return Source(self.data, offset, offset + size)

    def paths(self, index):
        source = self.section(str(index))
        paths = [decode_path(source.buffer()) for _ in range(source.varuint())]
        source.finish()
        if not paths:
            raise ValueError(f"Packed region has no paths: {self.names[index]}")
        return paths
