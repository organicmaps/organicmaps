#pragma once

#include "coding/varint.hpp"

#include "base/buffer_vector.hpp"

#include <algorithm>
#include <string>
#include <type_traits>
#include <vector>

namespace rw
{
template <class T, class TSink>
std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, void> Write(TSink & sink, T i)
{
  WriteVarUint(sink, i);
}

template <class T, class TSource>
std::enable_if_t<std::is_integral_v<T> && std::is_unsigned_v<T>, void> Read(TSource & src, T & i)
{
  i = ReadVarUint<T>(src);
}

template <class T, class TSink>
std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, void> Write(TSink & sink, T i)
{
  WriteVarInt(sink, i);
}

template <class T, class TSource>
std::enable_if_t<std::is_integral_v<T> && std::is_signed_v<T>, void> Read(TSource & src, T & i)
{
  i = ReadVarInt<T>(src);
}

template <class TSink>
void Write(TSink & sink, std::string const & s)
{
  uint32_t const count = static_cast<uint32_t>(s.size());
  WriteVarUint(sink, count);
  if (!s.empty())
    sink.Write(&s[0], count);
}

template <class TSource>
void Read(TSource & src, std::string & s)
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
}  // namespace impl

template <class TSink, class T>
void Write(TSink & sink, std::vector<T> const & v)
{
  impl::WriteCont(sink, v);
}

template <class TSource, class T>
void Read(TSource & src, std::vector<T> & v)
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

template <class Sink, class T>
void WritePOD(Sink & sink, T const & value)
{
  static_assert(std::is_trivially_copyable<T>::value, "");
  sink.Write(&value, sizeof(T));
}

template <class Sink, class T>
void ReadPOD(Sink & src, T & value)
{
  static_assert(std::is_trivially_copyable<T>::value, "");
  src.Read(&value, sizeof(T));
}

template <class TSource, class TCont>
void ReadVectorOfPOD(TSource & src, TCont & v)
{
  typedef typename TCont::value_type ValueT;
  /// This assert fails on std::pair<int, int> and OsmID class because std::pair is not trivially copyable:
  /// std::pair has a non-trivial copy-assignment and move-assignment operator.
  // static_assert(std::is_trivially_copyable_v<ValueT>);

  uint32_t const count = ReadVarUint<uint32_t>(src);
  if (count > 0)
  {
    v.resize(count);
    src.Read(&v[0], count * sizeof(ValueT));
  }
}

template <class TSink, class TCont>
void WriteVectorOfPOD(TSink & sink, TCont const & v)
{
  typedef typename TCont::value_type ValueT;
  /// This assert fails on std::pair<int, int> and OsmID class because std::pair is not trivially copyable:
  /// std::pair has a non-trivial copy-assignment and move-assignment operator.
  // static_assert(std::is_trivially_copyable_v<ValueT>);

  uint32_t const count = static_cast<uint32_t>(v.size());
  WriteVarUint(sink, count);

  if (count > 0)
    sink.Write(&v[0], count * sizeof(ValueT));
}

template <class ReaderT, class WriterT>
void ReadAndWrite(ReaderT & reader, WriterT & writer, size_t bufferSize = 4 * 1024)
{
  uint64_t size = reader.Size();
  std::vector<char> buffer(std::min(bufferSize, static_cast<size_t>(size)));

  while (size > 0)
  {
    size_t const curr = std::min(bufferSize, static_cast<size_t>(size));

    reader.Read(&buffer[0], curr);
    writer.Write(&buffer[0], curr);

    size -= curr;
  }
}
}  // namespace rw
