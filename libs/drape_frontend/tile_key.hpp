#pragma once

#include "drape_frontend/batcher_bucket.hpp"

#include "geometry/rect2d.hpp"
#include "geometry/screenbase.hpp"

#include "base/matrix.hpp"

#include <string>

namespace df
{
struct TileKey
{
  TileKey();
  TileKey(int x, int y, uint8_t zoomLevel);
  TileKey(TileKey const & key, uint64_t generation, uint64_t userMarksGeneration);

  // Operators < and == do not consider parameter m_generation.
  // m_generation is used to determine a generation of geometry for this tile key.
  // Geometry with different generations must be able to group by (x, y, zoomlevel).
  bool operator<(TileKey const & other) const;
  bool operator==(TileKey const & other) const;

  // These methods implement strict comparison of tile keys. It's necessary to merger of
  // batches which must not merge batches with different m_generation.
  bool LessStrict(TileKey const & other) const;
  bool EqualStrict(TileKey const & other) const;

  m2::RectD GetGlobalRect(bool clipByDataMaxZoom = true) const;

  math::Matrix<float, 4, 4> GetTileBasedModelView(ScreenBase const & screen) const;

  m2::PointI GetTileCoords() const;

  uint64_t GetHashValue(BatcherBucket bucket) const;

  std::string Coord2String() const;

  int m_x;
  int m_y;
  uint8_t m_zoomLevel;

  uint64_t m_generation;
  uint64_t m_userMarksGeneration;
};

struct TileKeyStrictComparator
{
  bool operator()(TileKey const & lhs, TileKey const & rhs) const { return lhs.LessStrict(rhs); }
};

std::string DebugPrint(TileKey const & key);
}  // namespace df
