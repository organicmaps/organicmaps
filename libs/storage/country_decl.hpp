#pragma once

#include "storage/storage_defines.hpp"

#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

#include "geometry/rect2d.hpp"

#include <string>

namespace storage
{
/// File name without extension (equal to english name - used in search for region).
struct CountryDef
{
  CountryDef() = default;
  CountryDef(CountryId const & countryId, m2::RectD const & rect) : m_countryId(countryId), m_rect(rect) {}

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
    if (east.IsRectInside(m_rect) || east.IsIntersect(m_rect))
      return true;
    m2::RectD const west(r.minX() - 360.0, r.minY(), r.maxX() - 360.0, r.maxY());
    return west.IsRectInside(m_rect) || west.IsIntersect(m_rect);
  }

  static m2::RectD SaveBoundRect() { return m2::RectD(-360, -180, 360, 180); }

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

template <class Source>
void Read(Source & src, CountryDef & p)
{
  rw::Read(src, p.m_countryId);

  auto const bounds = CountryDef::SaveBoundRect();
  auto const ptMin = PointUToPointD(Uint64ToPointUObsolete(ReadVarUint<uint64_t>(src)), kPointCoordBits, bounds);
  auto const ptMax = PointUToPointD(Uint64ToPointUObsolete(ReadVarUint<uint64_t>(src)), kPointCoordBits, bounds);

  p.m_rect = m2::RectD(ptMin, ptMax);
}

template <class Sink>
void Write(Sink & sink, CountryDef const & p)
{
  rw::Write(sink, p.m_countryId);

  auto const bounds = CountryDef::SaveBoundRect();
  WriteVarUint(sink,
               PointUToUint64Obsolete(PointDToPointU({p.m_rect.minX(), p.m_rect.minY()}, kPointCoordBits, bounds)));
  WriteVarUint(sink,
               PointUToUint64Obsolete(PointDToPointU({p.m_rect.maxX(), p.m_rect.maxY()}, kPointCoordBits, bounds)));
}
}  // namespace storage
