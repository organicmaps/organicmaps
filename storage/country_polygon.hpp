#pragma once

#include "storage/country_decl.hpp"

#include "coding/geometry_coding.hpp"
#include "coding/point_coding.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/varint.hpp"

namespace storage
{
template <class TSource>
void Read(TSource & src, CountryDef & p)
{
  rw::Read(src, p.m_countryId);

  pair<int64_t, int64_t> r;
  r.first = ReadVarInt<int64_t>(src);
  r.second = ReadVarInt<int64_t>(src);
  p.m_rect = Int64ToRectObsolete(r, serial::GeometryCodingParams().GetCoordBits());
}

template <class TSink>
void Write(TSink & sink, CountryDef const & p)
{
  rw::Write(sink, p.m_countryId);

  pair<int64_t, int64_t> const r =
      RectToInt64Obsolete(p.m_rect, serial::GeometryCodingParams().GetCoordBits());

  WriteVarInt(sink, r.first);
  WriteVarInt(sink, r.second);
}
}  // namespace storage
