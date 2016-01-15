#pragma once

#include "geometry/rect2d.hpp"

namespace df
{

enum class TileStatus
{
  // tile does not participate in rendering or fake
  Unknown = 0,
  // tile is rendered
  Rendered,
  // tile has been requested to be rendered
  Requested,
  // tile is ready but it was deferred for rendering
  Deferred
};

struct TileKey
{
  TileKey();
  TileKey(int x, int y, int zoomLevel);
  TileKey(TileKey const & key, uint64_t generation);

  // Operators < and == do not consider parameters m_styleZoomLevel and m_generation.
  // m_styleZoomLevel is used to work around FeaturesFetcher::ForEachFeatureID which is
  // not able to return correct rects on 18,19 zoom levels. So m_zoomLevel can not be
  // more than 17 for all subsystems except of styling.
  // m_generation is used to determine a generation of geometry for this tile key.
  // Geometry with different generations must be able to group by (x, y, zoomlevel).
  bool operator < (TileKey const & other) const;
  bool operator == (TileKey const & other) const;

  // This method implements strict comparison of tile keys. It's necessary to merger of
  // batches which must not merge batches with different m_styleZoomLevel and m_generation.
  bool LessStrict(TileKey const & other) const;

  m2::RectD GetGlobalRect(bool considerStyleZoom = false) const;

  int m_x;
  int m_y;
  int m_zoomLevel;

  int m_styleZoomLevel;
  uint64_t m_generation;
};

string DebugPrint(TileKey const & key);
string DebugPrint(TileStatus status);

} // namespace df
