#pragma once

#include "geometry/rect2d.hpp"

namespace df
{

struct TileKey
{
  TileKey();
  TileKey(int x, int y, int zoomLevel);
  TileKey(TileKey const & key, uint64_t generation);

  // Operators < and == do not consider parameter m_generation.
  // m_generation is used to determine a generation of geometry for this tile key.
  // Geometry with different generations must be able to group by (x, y, zoomlevel).
  bool operator < (TileKey const & other) const;
  bool operator == (TileKey const & other) const;

  // These methods implement strict comparison of tile keys. It's necessary to merger of
  // batches which must not merge batches with different m_generation.
  bool LessStrict(TileKey const & other) const;
  bool EqualStrict(TileKey const & other) const;

  m2::RectD GetGlobalRect(bool clipByDataMaxZoom = true) const;

  int m_x;
  int m_y;
  int m_zoomLevel;

  uint64_t m_generation;
};

struct TileKeyStrictComparator
{
  bool operator() (TileKey const & lhs, TileKey const & rhs) const
  {
    return lhs.LessStrict(rhs);
  }
};

string DebugPrint(TileKey const & key);

} // namespace df
