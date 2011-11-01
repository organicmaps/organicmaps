#pragma once

#include "../defines.hpp"

#include "../indexer/point_to_int64.hpp"
#include "../indexer/coding_params.hpp"

#include "../geometry/rect2d.hpp"

#include "../coding/read_write_utils.hpp"

namespace storage
{
  struct CountryDef
  {
    string m_name;
    m2::RectD m_rect;

    CountryDef() {}
    CountryDef(string const & name, m2::RectD const & r)
      : m_name(name), m_rect(r)
    {
    }
  };

  template <class TSource> void Read(TSource & src, CountryDef & p)
  {
    rw::Read(src, p.m_name);

    pair<int64_t, int64_t> r;
    r.first = ReadVarInt<int64_t>(src);
    r.second = ReadVarInt<int64_t>(src);
    p.m_rect = Int64ToRect(r, serial::CodingParams().GetCoordBits());
  }

  template <class TSink> void Write(TSink & sink, CountryDef const & p)
  {
    rw::Write(sink, p.m_name);

    pair<int64_t, int64_t> const r = RectToInt64(p.m_rect, serial::CodingParams().GetCoordBits());
    WriteVarInt(sink, r.first);
    WriteVarInt(sink, r.second);
  }
}
