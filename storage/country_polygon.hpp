#pragma once

#include "storage/country_decl.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

#include <cstdint>
#include <utility>

namespace storage
{
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

  std::pair<int64_t, int64_t> const r =
      RectToInt64Obsolete(p.m_rect, serial::GeometryCodingParams().GetCoordBits());

  WriteVarInt(sink, r.first);
  WriteVarInt(sink, r.second);
}
}  // namespace storage
