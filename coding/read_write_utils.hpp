#pragma once

#include "varint.hpp"

#include "../base/buffer_vector.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"


namespace rw
{
  template <class TSink>
  void Write(TSink & sink, uint32_t i)
  {
    WriteVarUint(sink, i);
  }

  template <class TSource>
  void Read(TSource & src, uint32_t & i)
  {
    i = ReadVarUint<uint32_t>(src);
  }

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

  namespace impl
  {
    template <class TSink, class TCont>
    void WriteCont(TSink & sink, TCont const & v)
    {
      uint32_t const count = static_cast<uint32_t>(v.size());
      WriteVarUint(sink, count);
      for (uint32_t i = 0; i < count; ++i)
        Write(sink, v[i]);
    }

    template <class TSource, class TCont>
    void ReadCont(TSource & src, TCont & v)
    {
      uint32_t const count = ReadVarUint<uint32_t>(src);
      v.resize(count);
      for (size_t i = 0; i < count; ++i)
        Read(src, v[i]);
    }
  }

  template <class TSink, class T>
  void Write(TSink & sink, vector<T> const & v)
  {
    impl::WriteCont(sink, v);
  }

  template <class TSource, class T>
  void Read(TSource & src, vector<T> & v)
  {
    impl::ReadCont(src, v);
  }

  template <class TSink, class T, size_t N>
  void Write(TSink & sink, buffer_vector<T, N> const & v)
  {
    impl::WriteCont(sink, v);
  }

  template <class TSource, class T, size_t N>
  void Read(TSource & src, buffer_vector<T, N> & v)
  {
    impl::ReadCont(src, v);
  }
}
