#pragma once

#include "coding/endianness.hpp"

#include "std/type_traits.hpp"

#include "3party/succinct/mappable_vector.hpp"
#include "3party/succinct/mapper.hpp"

namespace coding
{
template <typename T>
static T * Align8Ptr(T * ptr)
{
  uint64_t value = (reinterpret_cast<uint64_t>(ptr) + 0x7) & 0xfffffffffffffff8;
  return reinterpret_cast<T *>(value);
}

inline uint32_t NeedToAlign8(uint64_t written) { return 0x8 - (written & 0x7); }

class MapVisitor
{
public:
  MapVisitor(uint8_t const * base) : m_base(base), m_cur(m_base) {}

  template <typename T>
  typename enable_if<!is_pod<T>::value, MapVisitor &>::type operator()(
      T & val, char const * /* friendlyName */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  typename enable_if<is_pod<T>::value, MapVisitor &>::type operator()(
      T & val, char const * /* friendlyName */)
  {
    T const * valPtr = reinterpret_cast<T const *>(m_cur);
    val = *valPtr;
    m_cur += sizeof(T);

    m_cur = Align8Ptr(m_cur);
    return *this;
  }

  template <typename T>
  MapVisitor & operator()(succinct::mapper::mappable_vector<T> & vec,
                          char const * /* friendlyName */)
  {
    vec.clear();
    (*this)(vec.m_size, "size");

    vec.m_data = reinterpret_cast<const T *>(m_cur);
    size_t const bytes = vec.m_size * sizeof(T);

    m_cur += bytes;
    m_cur = Align8Ptr(m_cur);
    return *this;
  }

  size_t BytesRead() const { return static_cast<size_t>(m_cur - m_base); }

private:
  uint8_t const * const m_base;
  uint8_t const * m_cur;
};

class ReverseMapVisitor
{
public:
  ReverseMapVisitor(uint8_t * base) : m_base(base), m_cur(m_base) {}

  template <typename T>
  typename enable_if<!is_pod<T>::value, ReverseMapVisitor &>::type operator()(
      T & val, char const * /* friendlyName */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  typename enable_if<is_pod<T>::value, ReverseMapVisitor &>::type operator()(
      T & val, char const * /* friendlyName */)
  {
    T * valPtr = reinterpret_cast<T *>(m_cur);
    *valPtr = ReverseByteOrder(*valPtr);
    val = *valPtr;
    m_cur += sizeof(T);

    m_cur = Align8Ptr(m_cur);
    return *this;
  }

  template <typename T>
  ReverseMapVisitor & operator()(succinct::mapper::mappable_vector<T> & vec,
                                 char const * /* friendlyName */)
  {
    vec.clear();
    (*this)(vec.m_size, "size");

    vec.m_data = reinterpret_cast<const T *>(m_cur);
    for (auto const it = vec.begin(); it != vec.end(); ++it)
      *it = ReverseByteOrder(*it);
    size_t const bytes = vec.m_size * sizeof(T);

    m_cur += bytes;
    m_cur = Align8Ptr(m_cur);
    return *this;
  }

  size_t BytesRead() const { return static_cast<size_t>(m_cur - m_base); }

private:
  uint8_t * m_base;
  uint8_t * m_cur;
};

template <typename TWriter>
class FreezeVisitor
{
public:
  FreezeVisitor(TWriter & writer) : m_writer(writer), m_written(0) {}

  template <typename T>
  typename enable_if<!is_pod<T>::value, FreezeVisitor &>::type operator()(
      T & val, char const * /* friendlyName */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  typename enable_if<is_pod<T>::value, FreezeVisitor &>::type operator()(
      T & val, char const * /* friendlyName */)
  {
    m_writer.Write(reinterpret_cast<void const *>(&val), sizeof(T));
    m_written += sizeof(T);
    WritePadding();
    return *this;
  }

  template <typename T>
  FreezeVisitor & operator()(succinct::mapper::mappable_vector<T> & vec,
                             char const * /* friendlyName */)
  {
    (*this)(vec.m_size, "size");

    size_t const bytes = static_cast<size_t>(vec.m_size * sizeof(T));
    m_writer.Write(vec.m_data, static_cast<size_t>(bytes));
    m_written += bytes;
    WritePadding();
    return *this;
  }

  size_t Written() const { return m_written; }

private:
  void WritePadding()
  {
    uint32_t const padding = NeedToAlign8(m_written);
    static uint64_t const zero = 0;
    if (padding > 0 && padding < 8)
    {
      m_writer.Write(reinterpret_cast<void const *>(&zero), padding);
      m_written += padding;
    }
  }

  TWriter & m_writer;
  uint64_t m_written;
};

template <typename T>
size_t Map(T & value, uint8_t const * base, char const * friendlyName = "<TOP>")
{
  MapVisitor visitor(base);
  visitor(value, friendlyName);
  return visitor.BytesRead();
}

template <typename T>
size_t ReverseMap(T & value, uint8_t * base, char const * friendlyName = "<TOP>")
{
  ReverseMapVisitor visitor(base);
  visitor(value, friendlyName);
  return visitor.BytesRead();
}

template <typename T, typename TWriter>
size_t Freeze(T & val, TWriter & writer, char const * friendlyName = "<TOP>")
{
  FreezeVisitor<TWriter> visitor(writer);
  visitor(val, friendlyName);
  return visitor.Written();
}
}  // namespace coding
