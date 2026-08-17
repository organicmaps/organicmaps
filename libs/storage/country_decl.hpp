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

  /// @param[in] testRect is wrapped.
  /// @param[in] countryRect Country bound rect, non-wrapped, can cross antimeridian (CountryDef::m_rect).
  /// @return If testRect intersects countryRect or countryRect is inside testRect.
  enum class Overlap
  {
    NONE,
    INTERSECT,
    INSIDE
  };
  static Overlap IsIntersectOrInside(m2::RectD const & testRect, m2::RectD const & countryRect);

  bool IsRectOverlap(m2::RectD const & r) const { return IsIntersectOrInside(r, m_rect) != Overlap::NONE; }

  /// Calls \a fn(p1, p2) for each side of \a rect, but in _canonical_ coordinates (x in [-180, 180]).
  /// @param[in] rect is wrapped, but may cross the antimeridian (maxX > 180 or
  /// minX < -180). In that case it is split into its two canonical halves and 4 + 4 = 8 segments are
  /// emitted, so they can be tested against country polygons that always live in the canonical range.
  template <typename Fn>
  static void ForEachRectSideWrapped(m2::RectD const & rect, Fn && fn)
  {
    if (rect.maxX() > 180.0)
    {
      // [minX, 180] stays canonical; the part beyond 180 wraps to [-180, maxX - 360].
      m2::RectD(rect.minX(), rect.minY(), 180.0, rect.maxY()).ForEachSide(fn);
      m2::RectD(-180.0, rect.minY(), rect.maxX() - 360.0, rect.maxY()).ForEachSide(fn);
    }
    else if (rect.minX() < -180.0)
    {
      // [-180, maxX] stays canonical; the part below -180 wraps to [minX + 360, 180].
      m2::RectD(-180.0, rect.minY(), rect.maxX(), rect.maxY()).ForEachSide(fn);
      m2::RectD(rect.minX() + 360.0, rect.minY(), 180.0, rect.maxY()).ForEachSide(fn);
    }
    else
    {
      rect.ForEachSide(fn);
    }
  }

  static m2::RectD SaveBoundRect() { return m2::RectD(-360, -180, 360, 180); }

  CountryId m_countryId;
  m2::RectD m_rect;
};

inline std::string DebugPrint(CountryDef::Overlap o)
{
  switch (o)
  {
  case CountryDef::Overlap::NONE: return "NONE";
  case CountryDef::Overlap::INTERSECT: return "INTERSECT";
  case CountryDef::Overlap::INSIDE: return "INSIDE";
  }
  UNREACHABLE();
}

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
