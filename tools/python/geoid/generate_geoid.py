#!/usr/bin/env python3
"""Encode the pinned AndroidX geoid model as lossless numeric tiles.

The source is assets/database/geoid-height-map-v0.db in
androidx.core:core-location-altitude:1.0.1 (Apache-2.0). The SQLite asset has SHA-256
aa4f7a49d67ffd72df3c1e6ce3c2e8b5cd00d433f149591dae2ac25ff9b876d7.

Source: https://dl.google.com/dl/android/maven2/androidx/core/core-location-altitude/1.0.1/core-location-altitude-1.0.1.aar
Copyright 2022 The Android Open Source Project; see ../../../LICENSE for Apache-2.0.

Its six JPEG faces were decoded with libjpeg-turbo JDCT_ISLOW, then the PNG blue
channel corrections were added modulo 256. Runtime image decoding is deliberately
avoided: Android 5's decoder produces different samples from newer Android.

The geoid_model.bin asset pins these samples as 384 independent, lossless tiles. It has
384 little-endian uint16 stream lengths followed by zlib streams.
Within each 64x64 tile, samples are predicted by left + up - upper-left, modulo
256. --from-raw recreates the asset from the six face-major 512x512 byte grids using
only Python's standard library.

The numeric model and geoid_reference.csv are frozen one-off artifacts from the
pinned AndroidX version. Tools for extracting the raw grid from its JPEG/PNG
images and generating the independent AndroidX reference CSV are not included.
"""

import argparse
import hashlib
from pathlib import Path
import struct
import zlib


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
MODEL = ROOT / "android/sdk/src/main/assets/geoid_model.bin"
DECODED_SHA256 = "f296345ce9894951a53489a2abba02a27cbe962767a95581dc51a78977325a24"
FACES = 6
FACE_SIZE = 512
TILE_SIZE = 64
TILES_PER_FACE = FACE_SIZE // TILE_SIZE
TILE_COUNT = FACES * TILES_PER_FACE * TILES_PER_FACE
LENGTHS = struct.Struct(f"<{TILE_COUNT}H")


def tile_origins():
    for face in range(FACES):
        for row in range(0, FACE_SIZE, TILE_SIZE):
            for column in range(0, FACE_SIZE, TILE_SIZE):
                yield face, row, column


def validate_grid(grid):
    if len(grid) != FACES * FACE_SIZE * FACE_SIZE:
        raise ValueError("Expected six 512x512 byte grids")
    if hashlib.sha256(grid).hexdigest() != DECODED_SHA256:
        raise ValueError("Decoded model differs from the pinned reference")


def encode(grid):
    streams = []
    for face, row, column in tile_origins():
        residuals = bytearray()
        for y in range(TILE_SIZE):
            for x in range(TILE_SIZE):
                i = face * FACE_SIZE * FACE_SIZE + (row + y) * FACE_SIZE + column + x
                residuals.append((grid[i]
                                  - (grid[i - 1] if x else 0)
                                  - (grid[i - FACE_SIZE] if y else 0)
                                  + (grid[i - FACE_SIZE - 1] if x and y else 0)) & 255)
        streams.append(zlib.compress(residuals, 9))
    return streams


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--from-raw", type=Path, required=True, help="Recreate the asset from a decoded byte grid")
    args = parser.parse_args()
    grid = args.from_raw.read_bytes()
    validate_grid(grid)
    streams = encode(grid)
    MODEL.write_bytes(LENGTHS.pack(*(len(s) for s in streams)) + b"".join(streams))
    print(f"Encoded {TILE_COUNT} tiles, {sum(map(len, streams))} compressed bytes")


if __name__ == "__main__":
    main()
