#pragma once

#include "storage/storage_defines.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

#include "geometry/mercator.hpp"
#include "geometry/rect2d.hpp"

#include "base/bits.hpp"

#include <limits>
#include <string>

namespace storage
{
/// File name without extension (equal to english name - used in search for region).
struct CountryDef
{
  CountryDef() = default;
  CountryDef(CountryId const & countryId, m2::RectD const & rect) : m_countryId(countryId), m_rect(rect) {}

  /// Returns true if m_rect extends past the canonical [-180, 180] longitude range.
  bool HasExtendedRect() const { return m_rect.maxX() > 180.0 || m_rect.minX() < -180.0; }

  /// Point containment accounting for wrapped coordinates (both east and west shifts).
  /// Always checks +-360 so that extended query points (from wrapped viewports)
  /// match canonical country rects, and vice versa.
  bool IsPointInsideRect(m2::PointD const & pt) const
  {
    return m_rect.IsPointInside(pt) || m_rect.IsPointInside({pt.x + 360.0, pt.y}) ||
           m_rect.IsPointInside({pt.x - 360.0, pt.y});
  }

  /// Rect overlap (contains or intersects) accounting for wrapped coordinates.
  /// Always checks +-360 so that extended query rects (from wrapped viewports)
  /// match canonical country rects, and vice versa.
  bool IsRectOverlap(m2::RectD const & r) const
  {
    if (r.IsRectInside(m_rect) || r.IsIntersect(m_rect))
      return true;
    m2::RectD const east(r.minX() + 360.0, r.minY(), r.maxX() + 360.0, r.maxY());
    m2::RectD const west(r.minX() - 360.0, r.minY(), r.maxX() - 360.0, r.maxY());
    return east.IsRectInside(m_rect) || east.IsIntersect(m_rect) || west.IsRectInside(m_rect) ||
           west.IsIntersect(m_rect);
  }

  CountryId m_countryId;
  m2::RectD m_rect;
};

struct CountryInfo
{
  // @TODO(bykoianko) Twine will be used intead of this function.
  // So id (fName) will be converted to a local name.
  static void FileName2FullName(std::string & fName);
  static void FullName2GroupAndMap(std::string const & fName, std::string & group, std::string & map);

  /// Name (in native language) of country or region.
  /// (if empty - equals to file name of country - no additional memory)
  std::string m_name;
};

namespace country_decl_detail
{
// Custom encode/decode without coordinate clamping, to support extended rects
// past the antimeridian (e.g., [166, 186] for NZ Canterbury).
// Same varint/interleaving format as the obsolete functions — only the clamps are removed.

inline uint32_t DoubleToUint32Unclamped(double x, double min, double max, uint8_t coordBits)
{
  double const coordSize = static_cast<double>((uint64_t{1} << coordBits) - 1);
  double const result = 0.5 + (x - min) / (max - min) * coordSize;
  ASSERT_GREATER_OR_EQUAL(result, 0.0, (x, min, max));
  ASSERT_LESS_OR_EQUAL(result, static_cast<double>(std::numeric_limits<uint32_t>::max()), (x, min, max));
  return static_cast<uint32_t>(result);
}

inline double Uint32ToDoubleUnclamped(uint32_t x, double min, double max, uint8_t coordBits)
{
  double const coordSize = static_cast<double>((uint64_t{1} << coordBits) - 1);
  return min + static_cast<double>(x) * (max - min) / coordSize;
}

inline int64_t PointToInt64Unclamped(double x, double y, uint8_t coordBits)
{
  using mercator::Bounds;
  uint32_t const ix = DoubleToUint32Unclamped(x, Bounds::kMinX, Bounds::kMaxX, coordBits);
  uint32_t const iy = DoubleToUint32Unclamped(y, Bounds::kMinY, Bounds::kMaxY, coordBits);
  return static_cast<int64_t>(bits::BitwiseMerge(ix, iy));
}

inline m2::PointD Int64ToPointUnclamped(int64_t v, uint8_t coordBits)
{
  using mercator::Bounds;
  m2::PointU pt;
  bits::BitwiseSplit(static_cast<uint64_t>(v), pt.x, pt.y);
  return {Uint32ToDoubleUnclamped(pt.x, Bounds::kMinX, Bounds::kMaxX, coordBits),
          Uint32ToDoubleUnclamped(pt.y, Bounds::kMinY, Bounds::kMaxY, coordBits)};
}
}  // namespace country_decl_detail

template <class Source>
void Read(Source & src, CountryDef & p)
{
  rw::Read(src, p.m_countryId);

  std::pair<int64_t, int64_t> r;
  r.first = ReadVarInt<int64_t>(src);
  r.second = ReadVarInt<int64_t>(src);
  auto const coordBits = serial::GeometryCodingParams().GetCoordBits();
  m2::PointD const pt1 = country_decl_detail::Int64ToPointUnclamped(r.first, coordBits);
  m2::PointD const pt2 = country_decl_detail::Int64ToPointUnclamped(r.second, coordBits);
  p.m_rect = m2::RectD(pt1, pt2);
}

template <class Sink>
void Write(Sink & sink, CountryDef const & p)
{
  rw::Write(sink, p.m_countryId);

  auto const coordBits = serial::GeometryCodingParams().GetCoordBits();
  int64_t const p1 = country_decl_detail::PointToInt64Unclamped(p.m_rect.minX(), p.m_rect.minY(), coordBits);
  int64_t const p2 = country_decl_detail::PointToInt64Unclamped(p.m_rect.maxX(), p.m_rect.maxY(), coordBits);

  WriteVarInt(sink, p1);
  WriteVarInt(sink, p2);
}
}  // namespace storage
