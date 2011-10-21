#pragma once

#include "varint.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace rw
{
  template <class TSink>
  void Write(TSink & sink, string const & s)
  {
    uint32_t const count = static_cast<uint32_t>(s.size());
    WriteVarUint(sink, count);
    if (!s.empty())
      sink.Write(&s[0], count);
  }

  template <class TSource>
  void Read(TSource & src, string & s)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);
    s.resize(count);
    if (count > 0)
      src.Read(&s[0], count);
  }

  template <class TSink, class T>
  void Write(TSink & sink, vector<T> const & v)
  {
    uint32_t const count = static_cast<uint32_t>(v.size());
    WriteVarUint(sink, count);
    for (uint32_t i = 0; i < count; ++i)
      Write(sink, v[i]);
  }

  template <class TSource, class T>
  void Read(TSource & src, vector<T> & v)
  {
    uint32_t const count = ReadVarUint<uint32_t>(src);
    v.resize(count);
    for (size_t i = 0; i < count; ++i)
      Read(src, v[i]);
  }
}
