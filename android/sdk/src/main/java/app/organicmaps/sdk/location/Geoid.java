/*
 * Copyright 2022 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package app.organicmaps.sdk.location;

import androidx.annotation.VisibleForTesting;
import app.organicmaps.sdk.util.Assert;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Arrays;
import java.util.zip.DataFormatException;
import java.util.zip.Inflater;

/**
 * Geoid heights above WGS84, adapted from AndroidX AltitudeConverter/S2CellIdUtils to use direct
 * face coordinates and bundled numeric tiles, with a four-tile decoded cache.
 */
final class Geoid
{
  private static final double SCALE_METERS = 193.0;
  private static final double OFFSET_METERS = -107.0;
  private static final int FACE_BITS = 9;
  private static final int FACE_SIZE = 1 << FACE_BITS;
  private static final int FACE_MASK = FACE_SIZE - 1;
  private static final int LEAF_SIZE = 1 << 30;
  private static final int CELL_SHIFT = 30 - FACE_BITS;
  private static final int CELL_SIZE = 1 << CELL_SHIFT;
  private static final int TILE_BITS = 6;
  private static final int TILE_SIZE = 1 << TILE_BITS;
  private static final int TILE_MASK = TILE_SIZE - 1;
  private static final int TILES_PER_AXIS = FACE_SIZE / TILE_SIZE;
  private static final int TILES_PER_FACE = TILES_PER_AXIS * TILES_PER_AXIS;
  private static final int CACHE_TILES = 4;

  private final Inflater mInflater = new Inflater();
  private final byte[] mData;
  private final int[] mOffsets = new int[6 * TILES_PER_FACE + 1];
  private final byte[][] mTiles = new byte[CACHE_TILES][TILE_SIZE * TILE_SIZE];
  private final int[] mTileIds = new int[CACHE_TILES];
  private int mNextTile;

  Geoid(InputStream input) throws IOException
  {
    DataInputStream stream = new DataInputStream(input);
    byte[] lengths = new byte[2 * (mOffsets.length - 1)];
    stream.readFully(lengths);
    for (int tile = 0; tile < mOffsets.length - 1; ++tile)
      mOffsets[tile + 1] = mOffsets[tile] + (lengths[2 * tile] & 255) + ((lengths[2 * tile + 1] & 255) << 8);
    mData = new byte[mOffsets[mOffsets.length - 1]];
    stream.readFully(mData);
    Arrays.fill(mTileIds, -1);
  }

  double getHeight(double latitude, double longitude)
  {
    double lat = Math.toRadians(latitude);
    double lon = Math.toRadians(longitude);
    double cosLat = Math.cos(lat);
    double x = Math.cos(lon) * cosLat;
    double y = Math.sin(lon) * cosLat;
    double z = Math.sin(lat);
    int face = getFace(x, y, z);
    int leafI = toCoordinate(getU(face, x, y, z));
    int leafJ = toCoordinate(getV(face, x, y, z));
    int i = leafI >>> CELL_SHIFT;
    int j = leafJ >>> CELL_SHIFT;
    double wi = (leafI & (CELL_SIZE - 1)) / (double) CELL_SIZE;
    double wj = (leafJ & (CELL_SIZE - 1)) / (double) CELL_SIZE;
    int node = toNode(face, i, j);
    if (i < FACE_MASK && j < FACE_MASK)
      return interpolate(node, node + 1, node + FACE_SIZE, node + FACE_SIZE + 1, wi, wj);

    return interpolateBoundary(face, i, j, wi, wj);
  }

  private double interpolate(int n0, int n1, int n2, int n3, double wi, double wj)
  {
    double h0 = getSample(n0);
    double h1 = getSample(n1);
    double h2 = getSample(n2);
    double h3 = getSample(n3);
    double value = h0 + (h1 - h0) * wi + (h2 - h0) * wj + (h3 - h1 - h2 + h0) * wi * wj;
    return value / 255 * SCALE_METERS + OFFSET_METERS;
  }

  // Packed node order is face, row, column. Keeping samples in this order avoids S2 Hilbert IDs.
  @VisibleForTesting
  int getSample(int node)
  {
    int face = node >>> (2 * FACE_BITS);
    int i = node & FACE_MASK;
    int j = (node >>> FACE_BITS) & FACE_MASK;
    int tile = face * TILES_PER_FACE + (j >>> TILE_BITS) * TILES_PER_AXIS + (i >>> TILE_BITS);
    int slot = 0;
    while (slot < mTiles.length && mTileIds[slot] != tile)
      ++slot;
    if (slot == mTiles.length)
    {
      slot = mNextTile;
      mNextTile = (mNextTile + 1) % CACHE_TILES;
      decodeTile(tile, mTiles[slot]);
      mTileIds[slot] = tile;
    }
    return mTiles[slot][(j & TILE_MASK) * TILE_SIZE + (i & TILE_MASK)] & 255;
  }

  private void decodeTile(int tile, byte[] output)
  {
    mInflater.reset();
    try
    {
      mInflater.setInput(mData, mOffsets[tile], mOffsets[tile + 1] - mOffsets[tile]);
      int size = mInflater.inflate(output);
      Assert.debug(size == output.length && mInflater.finished(), "Invalid bundled geoid tile");
    }
    catch (DataFormatException e)
    {
      throw new IllegalStateException("Invalid bundled geoid tile: " + tile, e);
    }

    for (int y = 0; y < TILE_SIZE; ++y)
    {
      for (int x = 0; x < TILE_SIZE; ++x)
      {
        int index = y * TILE_SIZE + x;
        int left = x == 0 ? 0 : output[index - 1] & 255;
        int up = y == 0 ? 0 : output[index - TILE_SIZE] & 255;
        int upperLeft = x == 0 || y == 0 ? 0 : output[index - TILE_SIZE - 1] & 255;
        output[index] += left + up - upperLeft;
      }
    }
  }

  private static int getFace(double x, double y, double z)
  {
    double ax = Math.abs(x);
    double ay = Math.abs(y);
    double az = Math.abs(z);
    if (ax > ay)
    {
      if (ax > az)
        return x < 0 ? 3 : 0;
      return z < 0 ? 5 : 2;
    }
    if (ay > az)
      return y < 0 ? 4 : 1;
    return z < 0 ? 5 : 2;
  }

  private static double getU(int face, double x, double y, double z)
  {
    return switch (face)
    {
      case 0 -> y / x;
      case 1 -> -x / y;
      case 2 -> -x / z;
      case 3 -> z / x;
      case 4 -> z / y;
      default -> -y / z;
    };
  }

  private static double getV(int face, double x, double y, double z)
  {
    return switch (face)
    {
      case 0 -> z / x;
      case 1 -> z / y;
      case 2 -> -y / z;
      case 3 -> y / x;
      case 4 -> -x / y;
      default -> -x / z;
    };
  }

  private static int toCoordinate(double uv)
  {
    double s = uv >= 0 ? 0.5 * Math.sqrt(1 + 3 * uv) : 1 - 0.5 * Math.sqrt(1 - 3 * uv);
    return Math.max(0, Math.min(LEAF_SIZE - 1, (int) Math.round(LEAF_SIZE * s - 0.5)));
  }

  private static int toNode(int face, int i, int j)
  {
    return (face << (2 * FACE_BITS)) | (j << FACE_BITS) | i;
  }

  private double interpolateBoundary(int face, int i, int j, double wi, double wj)
  {
    // Preserve AndroidX interpolation across cube faces: clamping and extrapolating from the
    // last interior cell can change edge heights by several meters.
    int node = toNode(face, i, j);
    int[] edges = getNeighbors(node);
    int[] down = getNeighbors(edges[0]);
    int[] right = getNeighbors(edges[1]);
    int[] up = getNeighbors(edges[2]);
    int[] left = getNeighbors(edges[3]);
    int sw = findCommon(down, right, node);
    int se = findCommon(right, up, node);
    int ne = findCommon(up, left, node);
    int n1 = face % 2 == 1 && i == FACE_MASK ? sw : edges[1];
    int n2 = face % 2 == 0 && j == FACE_MASK ? ne : edges[2];
    int n3 = n1 == sw ? edges[1] : n2 == ne ? edges[2] : se;
    return interpolate(node, n1, n2, n3, wi, wj);
  }

  private static int[] getNeighbors(int node)
  {
    return new int[] {getNeighbor(node, 0, -1), getNeighbor(node, 1, 0), getNeighbor(node, 0, 1),
                      getNeighbor(node, -1, 0)};
  }

  private static int findCommon(int[] a, int[] b, int node)
  {
    for (int x : a)
      if (x != node)
        for (int y : b)
          if (x == y)
            return x;
    return node;
  }

  private static int getNeighbor(int node, int di, int dj)
  {
    int face = node >>> (2 * FACE_BITS);
    int i = node & FACE_MASK;
    int j = (node >>> FACE_BITS) & FACE_MASK;
    int ni = i + di;
    int nj = j + dj;
    if (ni >= 0 && ni < FACE_SIZE && nj >= 0 && nj < FACE_SIZE)
      return toNode(face, ni, nj);

    // S2 wraps a cell's center onto the neighboring cube face using a linear UV projection.
    double ci = Math.max(-1, Math.min(LEAF_SIZE, (long) i * CELL_SIZE + CELL_SIZE / 2 + (long) di * CELL_SIZE));
    double cj = Math.max(-1, Math.min(LEAF_SIZE, (long) j * CELL_SIZE + CELL_SIZE / 2 + (long) dj * CELL_SIZE));
    double limit = Math.nextUp(1.0);
    double u = Math.max(-limit, Math.min(limit, (2 * ci + 1 - LEAF_SIZE) / LEAF_SIZE));
    double v = Math.max(-limit, Math.min(limit, (2 * cj + 1 - LEAF_SIZE) / LEAF_SIZE));
    double x;
    double y;
    double z;
    switch (face)
    {
    case 0 ->
    {
      x = 1;
      y = u;
      z = v;
    }
    case 1 ->
    {
      x = -u;
      y = 1;
      z = v;
    }
    case 2 ->
    {
      x = -u;
      y = -v;
      z = 1;
    }
    case 3 ->
    {
      x = -1;
      y = -v;
      z = -u;
    }
    case 4 ->
    {
      x = v;
      y = -1;
      z = -u;
    }
    default ->
    {
      x = v;
      y = u;
      z = -1;
    }
    }
    int newFace = getFace(x, y, z);
    int newI =
        Math.max(0, Math.min(LEAF_SIZE - 1, (int) Math.round(LEAF_SIZE * 0.5 * (getU(newFace, x, y, z) + 1) - 0.5)));
    int newJ =
        Math.max(0, Math.min(LEAF_SIZE - 1, (int) Math.round(LEAF_SIZE * 0.5 * (getV(newFace, x, y, z) + 1) - 0.5)));
    return toNode(newFace, newI >>> CELL_SHIFT, newJ >>> CELL_SHIFT);
  }
}
