#include "drape_frontend/tile_key.hpp"

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "indexer/scales.hpp"

#include "geometry/mercator.hpp"

#include <sstream>

namespace df
{
TileKey::TileKey()
  : m_x(-1), m_y(-1),
    m_zoomLevel(-1),
    m_generation(0),
    m_userMarksGeneration(0)
{}

TileKey::TileKey(int x, int y, int zoomLevel)
  : m_x(x), m_y(y),
    m_zoomLevel(zoomLevel),
    m_generation(0),
    m_userMarksGeneration(0)
{}

TileKey::TileKey(TileKey const & key, uint64_t generation, uint64_t userMarksGeneration)
  : m_x(key.m_x)
  , m_y(key.m_y)
  , m_zoomLevel(key.m_zoomLevel)
  , m_generation(generation)
  , m_userMarksGeneration(userMarksGeneration)
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
  if (m_userMarksGeneration != other.m_userMarksGeneration)
    return m_userMarksGeneration < other.m_userMarksGeneration;

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
         m_generation == other.m_generation &&
         m_userMarksGeneration == other.m_userMarksGeneration;
}

m2::RectD TileKey::GetGlobalRect(bool clipByDataMaxZoom) const
{
  int const zoomLevel = clipByDataMaxZoom ? ClipTileZoomByMaxDataZoom(m_zoomLevel) : m_zoomLevel;
  ASSERT_GREATER(zoomLevel, 0, ());
  double const worldSizeDivisor = 1 << (zoomLevel - 1);
  // Mercator SizeX and SizeY are equal.
  double const rectSize = MercatorBounds::kRangeX / worldSizeDivisor;

  double const startX = m_x * rectSize;
  double const startY = m_y * rectSize;

  return m2::RectD(startX, startY, startX + rectSize, startY + rectSize);
}

m2::PointI TileKey::GetTileCoords() const
{
  return m2::PointI(m_x, m_y);
}

math::Matrix<float, 4, 4> TileKey::GetTileBasedModelView(ScreenBase const & screen) const
{
  return screen.GetModelView(GetGlobalRect().Center(), kShapeCoordScalar);
}

std::string DebugPrint(TileKey const & key)
{
  std::ostringstream out;
  out << "[x = " << key.m_x << ", y = " << key.m_y << ", zoomLevel = "
      << key.m_zoomLevel << ", gen = " << key.m_generation
      << ", user marks gen = " << key.m_userMarksGeneration << "]";
  return out.str();
}
}  // namespace df
