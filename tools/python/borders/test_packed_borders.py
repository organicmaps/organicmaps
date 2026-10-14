from pathlib import Path
import struct
import tempfile
import unittest

import numpy as np

from packed_borders import COORD_LIMIT, PackedBorders, Source, decode_path, predict_polyline


def varuint(value):
    result = bytearray()
    while value >= 128:
        result.append((value & 127) | 128)
        value >>= 7
    return bytes(result + bytes([value]))


def string(value):
    data = value.encode("utf-8")
    return varuint(len(data)) + data


def merge(x, y):
    return sum(((x >> i) & 1) << (2 * i) | ((y >> i) & 1) << (2 * i + 1) for i in range(32))


def delta(point, prediction):
    x, y = (int(point[i]) - int(prediction[i]) for i in (0, 1))
    return merge((x << 1) ^ (x >> 31), (y << 1) ^ (y >> 31))


def buffer(data):
    return varuint(len(data)) + data


def container(sections):
    data = bytearray(b"\0" * 8)
    entries = []
    for name, section in sections:
        entries.append((name, len(data), len(section)))
        data.extend(section)
    directory = len(data)
    data.extend(varuint(len(entries)))
    for name, offset, size in entries:
        data.extend(string(name) + varuint(offset) + varuint(size))
    struct.pack_into("<Q", data, 0, directory)
    return bytes(data)


def path_buffer(points):
    data = bytearray()
    for i, point in enumerate(points):
        prediction = (0, 0) if not i else points[0] if i == 1 else predict_polyline(points[i - 1], points[i - 2])
        data.extend(varuint(delta(point, prediction)))
    return buffer(data)


def packed_fixture(regions, bounds=None, bounds_format="wrapped"):
    """FilesContainer/Prev2/CountryDef with opaque ignored display sections."""
    sections = []
    info = bytearray(varuint(len(regions)))
    for index, (name, coordinates) in enumerate(regions.items()):
        points = np.floor((np.asarray(coordinates) + 180.0) / 360.0 * COORD_LIMIT + 0.5).astype(np.uint32)
        sections.append((str(index), varuint(1) + path_buffer(points)))
        sections.append(("t" + str(index), b"ignored display bytes"))
        left, bottom = np.min(coordinates, axis=0) - 0.001
        right, top = np.max(coordinates, axis=0) + 0.001
        rect = (bounds or {}).get(name, (left, bottom, right, top))
        info.extend(string(name))
        for x, y in (rect[:2], rect[2:]):
            longitude_limit = 180.0 if bounds_format == "legacy" else 360.0
            xi = int(np.floor((x + longitude_limit) / (2 * longitude_limit) * COORD_LIMIT + 0.5))
            yi = int(np.floor((y + 180.0) / 360.0 * COORD_LIMIT + 0.5))
            point = merge(xi, yi)
            info.extend(varuint(point << 1 if bounds_format == "legacy" else point))
    return container(sections + [("info", bytes(info))])


class PackedDecoderTests(unittest.TestCase):
    def test_polyline_prediction_clamps_at_grid_edges(self):
        points = [(0, COORD_LIMIT), (COORD_LIMIT, 0), (1, 2), (0, COORD_LIMIT)]
        encoded = path_buffer(points)
        result = decode_path(Source(encoded).buffer())
        np.testing.assert_array_equal(result, points)

    def test_metadata_wraps_longitude_beyond180(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "packed.bin"
            regions = {"Fiji": [(179.0, 0), (179.5, 0), (179.5, 1), (179.0, 1)]}
            path.write_bytes(packed_fixture(regions, {"Fiji": (178, -1, 182, 2)}))
            decoded = PackedBorders(path)
            self.assertEqual(decoded.names, ["Fiji"])
            self.assertAlmostEqual(decoded.bounds["Fiji"][2], 182, places=5)
            self.assertEqual(len(decoded.paths(0)[0]), 4)

    def test_missing_section_or_truncated_geometry_is_rejected(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "packed.bin"
            data = packed_fixture({"A": [(0, 0), (1, 0), (0, 1)]})
            path.write_bytes(data[:-1])
            with self.assertRaisesRegex(ValueError, "Truncated"):
                PackedBorders(path)
            path.write_bytes(data)
            with self.assertRaisesRegex(ValueError, "Missing packed border section"):
                PackedBorders(path).section("missing")

    def test_legacy_signed_rectangle_points_use_old_longitude_range(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "packed.bin"
            regions = {"A": [(5, 10), (6, 10), (6, 11), (5, 11)]}
            path.write_bytes(packed_fixture(regions, {"A": (4, 9, 7, 12)}, bounds_format="legacy"))
            decoded = PackedBorders(path, "legacy")
            np.testing.assert_allclose(decoded.bounds["A"], (4, 9, 7, 12), atol=2e-7)
