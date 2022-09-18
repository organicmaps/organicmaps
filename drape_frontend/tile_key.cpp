#include "drape_frontend/tile_key.hpp"

#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/tile_utils.hpp"

#include "geometry/mercator.hpp"

#include <sstream>

namespace df
{
namespace
{
uint32_t constexpr GetMask(uint8_t bitsCount)
{
  ASSERT(bitsCount > 0 && bitsCount < 32, (bitsCount));
  return (uint32_t(1) << bitsCount) - 1;
}
}  // namespace

TileKey::TileKey()
  : m_x(-1), m_y(-1),
    m_zoomLevel(0),
    m_generation(0),
    m_userMarksGeneration(0)
{}

TileKey::TileKey(int x, int y, uint8_t zoomLevel)
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
  double const rectSize = mercator::Bounds::kRangeX / worldSizeDivisor;

  double const startX = m_x * rectSize;
  double const startY = m_y * rectSize;

  return m2::RectD(startX, startY, startX + rectSize, startY + rectSize);
}

m2::PointI TileKey::GetTileCoords() const
{
  return m2::PointI(m_x, m_y);
}

uint64_t TileKey::GetHashValue(BatcherBucket bucket) const
{
  // Format (from most significant to least):
  // 8 bit - generation mod 2^8;
  // 8 bit - user marks generation mod 2^8;
  // 5 bit - zoom level;
  // 20 bit - y;
  // 20 bit - x;
  // 3 bit - bucket.

  uint8_t constexpr kCoordsBits = 20;
  uint8_t constexpr kZoomBits = 5;
  uint8_t constexpr kGenerationBits = 8;
  uint8_t constexpr kBucketBits = 3;
  uint32_t constexpr kCoordsMask = GetMask(kCoordsBits);
  uint32_t constexpr kZoomMask = GetMask(kZoomBits);
  uint32_t constexpr kBucketMask = GetMask(kBucketBits);
  uint32_t constexpr kGenerationMod = 1 << kGenerationBits;

  // Transform [-b, b] coordinates range -> [0, 2b] positive coordinates range.
  int constexpr kCoordsOffset = 1 << (kCoordsBits - 1);
  CHECK(abs(m_x) <= kCoordsOffset, (m_x));
  CHECK(abs(m_y) <= kCoordsOffset, (m_y));
  auto const x = static_cast<uint64_t>(m_x + kCoordsOffset) & kCoordsMask;
  auto const y = static_cast<uint64_t>(m_y + kCoordsOffset) & kCoordsMask;

  CHECK(m_zoomLevel <= kZoomMask, (m_zoomLevel));
  uint64_t const zoom = static_cast<uint64_t>(m_zoomLevel) & kZoomMask;

  auto const umg = static_cast<uint64_t>(m_userMarksGeneration % kGenerationMod);
  auto const g = static_cast<uint64_t>(m_generation % kGenerationMod);

  auto const hash = x | (y << kCoordsBits) | (zoom << (2 * kCoordsBits)) |
                    (umg << (2 * kCoordsBits + kZoomBits)) |
                    (g << (2 * kCoordsBits + kZoomBits + kGenerationBits));

  return (hash << kBucketBits) | (static_cast<uint64_t>(bucket) & kBucketMask);
}

math::Matrix<float, 4, 4> TileKey::GetTileBasedModelView(ScreenBase const & screen) const
{
  return screen.GetModelView(GetGlobalRect().Center(), kShapeCoordScalar);
}

std::string TileKey::Coord2String() const
{
  using namespace strings;
  return to_string(m_x) + ' ' + to_string(m_y) + ' ' + to_string(int(m_zoomLevel));
}

std::string DebugPrint(TileKey const & key)
{
  std::ostringstream out;
  out << "{ x = " << key.m_x << ", y = " << key.m_y << ", zoomLevel = "
      << (int)key.m_zoomLevel << ", gen = " << key.m_generation
      << ", user marks gen = " << key.m_userMarksGeneration << " }";
  return out.str();
}
}  // namespace df
