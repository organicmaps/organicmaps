#pragma once

#include "storage/storage_defines.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

#include "geometry/rect2d.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace storage
{
/// File name without extension (equal to english name - used in search for region).
struct CountryDef
{
  CountryDef() = default;
  CountryDef(CountryId const & countryId, m2::RectD const & rect) : m_countryId(countryId), m_rect(rect) {}

  CountryId m_countryId;
  m2::RectD m_rect;
};

struct CountryInfo
{
  CountryInfo() = default;
  CountryInfo(std::string const & id) : m_name(id) {}

  // @TODO(bykoianko) Twine will be used intead of this function.
  // So id (fName) will be converted to a local name.
  static void FileName2FullName(std::string & fName);
  static void FullName2GroupAndMap(std::string const & fName, std::string & group, std::string & map);

  bool IsNotEmpty() const { return !m_name.empty(); }

  /// Name (in native language) of country or region.
  /// (if empty - equals to file name of country - no additional memory)
  std::string m_name;
};

template <class Source>
void Read(Source & src, CountryDef & p)
{
  rw::Read(src, p.m_countryId);

  std::pair<int64_t, int64_t> r;
  r.first = ReadVarInt<int64_t>(src);
  r.second = ReadVarInt<int64_t>(src);
  p.m_rect = Int64ToRectObsolete(r, serial::GeometryCodingParams().GetCoordBits());
}

template <class Sink>
void Write(Sink & sink, CountryDef const & p)
{
  rw::Write(sink, p.m_countryId);

  std::pair<int64_t, int64_t> const r = RectToInt64Obsolete(p.m_rect, serial::GeometryCodingParams().GetCoordBits());

  WriteVarInt(sink, r.first);
  WriteVarInt(sink, r.second);
}
}  // namespace storage
