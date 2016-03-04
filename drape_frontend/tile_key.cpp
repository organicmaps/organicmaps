#include "drape_frontend/tile_key.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

namespace df
{

TileKey::TileKey()
  : m_x(-1), m_y(-1),
    m_zoomLevel(-1),
    m_generation(0)
{}

TileKey::TileKey(int x, int y, int zoomLevel)
  : m_x(x), m_y(y),
    m_zoomLevel(zoomLevel),
    m_generation(0)
{}

TileKey::TileKey(TileKey const & key, uint64_t generation)
  : m_x(key.m_x), m_y(key.m_y),
    m_zoomLevel(key.m_zoomLevel),
    m_generation(generation)
{}

bool TileKey::operator <(TileKey const & other) const
{
  if (m_zoomLevel != other.m_zoomLevel)
    return m_zoomLevel < other.m_zoomLevel;

  if (m_y != other.m_y)
    return m_y < other.m_y;

  return m_x < other.m_x;
}

bool TileKey::operator ==(TileKey const & other) const
{
  return m_x == other.m_x &&
         m_y == other.m_y &&
         m_zoomLevel == other.m_zoomLevel;
}

bool TileKey::LessStrict(TileKey const & other) const
{
  if (m_generation != other.m_generation)
    return m_generation < other.m_generation;

  if (m_zoomLevel != other.m_zoomLevel)
    return m_zoomLevel < other.m_zoomLevel;

  if (m_y != other.m_y)
    return m_y < other.m_y;

  return m_x < other.m_x;
}

bool TileKey::EqualStrict(TileKey const & other) const
{
  return m_x == other.m_x &&
         m_y == other.m_y &&
         m_zoomLevel == other.m_zoomLevel &&
         m_generation == other.m_generation;
}

m2::RectD TileKey::GetGlobalRect(bool clipByDataMaxZoom) const
{
  int const zoomLevel = clipByDataMaxZoom ? ClipTileZoomByMaxDataZoom(m_zoomLevel) : m_zoomLevel;
  ASSERT_GREATER(zoomLevel, 0, ());
  double const worldSizeDevisor = 1 << (zoomLevel - 1);
  // Mercator SizeX and SizeY are equal.
  double const rectSize = (MercatorBounds::maxX - MercatorBounds::minX) / worldSizeDevisor;

  double const startX = m_x * rectSize;
  double const startY = m_y * rectSize;

  return m2::RectD (startX, startY, startX + rectSize, startY + rectSize);
}

string DebugPrint(TileKey const & key)
{
  ostringstream out;
  out << "[x = " << key.m_x << ", y = " << key.m_y << ", zoomLevel = "
      << key.m_zoomLevel << ", gen = " << key.m_generation << "]";
  return out.str();
}

} //namespace df
