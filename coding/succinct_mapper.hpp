#pragma once

#include "coding/endianness.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include "std/type_traits.hpp"

#include "3party/succinct/mappable_vector.hpp"
#include "3party/succinct/mapper.hpp"

namespace coding
{
template <typename T>
static T * Align8Ptr(T * ptr)
{
  uint64_t const value = (reinterpret_cast<uint64_t>(ptr) + 0x7) & 0xfffffffffffffff8;
  return reinterpret_cast<T *>(value);
}

inline uint32_t ToAlign8(uint64_t written) { return (0x8 - (written & 0x7)) & 0x7; }

class MapVisitor
{
public:
  explicit MapVisitor(uint8_t const * base) : m_base(base), m_cur(m_base) {}

  template <typename T>
  typename enable_if<!is_pod<T>::value, MapVisitor &>::type operator()(T & val,
                                                                       char const * /* name */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  typename enable_if<is_pod<T>::value, MapVisitor &>::type operator()(T & val,
                                                                      char const * /* name */)
  {
    T const * valPtr = reinterpret_cast<T const *>(m_cur);
    val = *valPtr;

    m_cur = Align8Ptr(m_cur + sizeof(T));
    return *this;
  }

  template <typename T>
  MapVisitor & operator()(succinct::mapper::mappable_vector<T> & vec, char const * /* name */)
  {
    vec.clear();
    (*this)(vec.m_size, "size");
    vec.m_data = reinterpret_cast<const T *>(m_cur);

    m_cur = Align8Ptr(m_cur + vec.m_size * sizeof(T));
    return *this;
  }

  uint64_t BytesRead() const { return static_cast<uint64_t>(m_cur - m_base); }

private:
  uint8_t const * const m_base;
  uint8_t const * m_cur;

  DISALLOW_COPY_AND_MOVE(MapVisitor);
};

class ReverseMapVisitor
{
public:
  explicit ReverseMapVisitor(uint8_t * base) : m_base(base), m_cur(m_base) {}

  template <typename T>
  typename enable_if<!is_pod<T>::value, ReverseMapVisitor &>::type operator()(
      T & val, char const * /* name */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  typename enable_if<is_pod<T>::value, ReverseMapVisitor &>::type operator()(
      T & val, char const * /* name */)
  {
    T * valPtr = reinterpret_cast<T *>(m_cur);
    *valPtr = ReverseByteOrder(*valPtr);
    val = *valPtr;

    m_cur = Align8Ptr(m_cur + sizeof(T));
    return *this;
  }

  template <typename T>
  ReverseMapVisitor & operator()(succinct::mapper::mappable_vector<T> & vec,
                                 char const * /* name */)
  {
    vec.clear();
    (*this)(vec.m_size, "size");

    vec.m_data = reinterpret_cast<const T *>(m_cur);
    for (auto const it = vec.cbegin(); it != vec.cend(); ++it)
      *it = ReverseByteOrder(*it);

    m_cur = Align8Ptr(m_cur + vec.m_size * sizeof(T));
    return *this;
  }

  uint64_t BytesRead() const { return static_cast<uint64_t>(m_cur - m_base); }

private:
  uint8_t * const m_base;
  uint8_t * m_cur;

  DISALLOW_COPY_AND_MOVE(ReverseMapVisitor);
};

template <typename TWriter>
class FreezeVisitor
{
public:
  explicit FreezeVisitor(TWriter & writer) : m_writer(writer), m_bytesWritten(0) {}

  template <typename T>
  typename enable_if<!is_pod<T>::value, FreezeVisitor &>::type operator()(T & val,
                                                                          char const * /* name */)
  {
    ASSERT(IsAligned(), ());
    val.map(*this);
    return *this;
  }

  template <typename T>
  typename enable_if<is_pod<T>::value, FreezeVisitor &>::type operator()(T & val,
                                                                         char const * /* name */)
  {
    ASSERT(IsAligned(), ());
    m_writer.Write(&val, sizeof(T));
    m_bytesWritten += sizeof(T);
    WritePadding();
    return *this;
  }

  template <typename T>
  FreezeVisitor & operator()(succinct::mapper::mappable_vector<T> & vec, char const * /* name */)
  {
    ASSERT(IsAligned(), ());
    (*this)(vec.m_size, "size");

    size_t const bytes = static_cast<size_t>(vec.m_size * sizeof(T));
    m_writer.Write(vec.m_data, bytes);
    m_bytesWritten += bytes;
    WritePadding();
    return *this;
  }

  uint64_t BytesWritten() const { return m_bytesWritten; }

private:
  bool IsAligned() const { return ToAlign8(m_writer.Pos()) == 0; }

  void WritePadding()
  {
    static uint64_t const zero = 0;

    uint32_t const padding = ToAlign8(m_bytesWritten);
    if (padding == 0)
      return;
    m_writer.Write(&zero, padding);
    m_bytesWritten += padding;
  }

  TWriter & m_writer;
  uint64_t m_bytesWritten;

  DISALLOW_COPY_AND_MOVE(FreezeVisitor);
};

template <typename T>
uint64_t Map(T & value, uint8_t const * base, char const * name)
{
  MapVisitor visitor(base);
  visitor(value, name);
  return visitor.BytesRead();
}

template <typename T>
uint64_t ReverseMap(T & value, uint8_t * base, char const * name)
{
  ReverseMapVisitor visitor(base);
  visitor(value, name);
  return visitor.BytesRead();
}

template <typename T, typename TWriter>
uint64_t Freeze(T & val, TWriter & writer, char const * name)
{
  FreezeVisitor<TWriter> visitor(writer);
  visitor(val, name);
  return visitor.BytesWritten();
}
}  // namespace coding
