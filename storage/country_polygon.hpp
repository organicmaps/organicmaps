#pragma once

#include "storage/country_decl.hpp"

#include "indexer/point_to_int64.hpp"
#include "indexer/coding_params.hpp"

#include "coding/read_write_utils.hpp"


namespace storage
{
  template <class TSource> void Read(TSource & src, CountryDef & p)
  {
    rw::Read(src, p.m_countryId);

    pair<int64_t, int64_t> r;
    r.first = ReadVarInt<int64_t>(src);
    r.second = ReadVarInt<int64_t>(src);
    p.m_rect = Int64ToRect(r, serial::CodingParams().GetCoordBits());
  }

  template <class TSink> void Write(TSink & sink, CountryDef const & p)
  {
    rw::Write(sink, p.m_countryId);

    pair<int64_t, int64_t> const r = RectToInt64(p.m_rect, serial::CodingParams().GetCoordBits());
    WriteVarInt(sink, r.first);
    WriteVarInt(sink, r.second);
  }
}
