#pragma once

#include "coding/endianness.hpp"

#include "base/assert.hpp"
#include "base/macros.hpp"

#include <type_traits>

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-local-typedef"
#endif

#include "3party/succinct/mappable_vector.hpp"
#include "3party/succinct/mapper.hpp"

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

namespace coding
{
template <typename T>
static T * Align8Ptr(T * ptr)
{
  uint64_t const value = (reinterpret_cast<uint64_t>(ptr) + 0x7) & 0xfffffffffffffff8;
  return reinterpret_cast<T *>(value);
}

inline uint32_t ToAlign8(uint64_t written)
{
  return (0x8 - (written & 0x7)) & 0x7;
}

inline bool IsAlign8(uint64_t offset)
{
  return ToAlign8(offset) == 0;
}

template <typename TWriter>
void WritePadding(TWriter & writer, uint64_t & bytesWritten)
{
  static uint64_t const zero = 0;

  uint32_t const padding = ToAlign8(bytesWritten);
  if (padding == 0)
    return;
  writer.Write(&zero, padding);
  bytesWritten += padding;
}

template <typename Source>
void SkipPadding(Source & src)
{
  uint32_t const padding = ToAlign8(src.Pos());
  src.Skip(padding);
}

class MapVisitor
{
public:
  explicit MapVisitor(uint8_t const * base) : m_base(base), m_cur(m_base) {}

  template <typename T>
  std::enable_if_t<!std::is_trivial_v<T>, MapVisitor &> operator()(T & val, char const * /* name */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  std::enable_if_t<std::is_trivial_v<T>, MapVisitor &> operator()(T & val, char const * /* name */)
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
    vec.m_data = reinterpret_cast<T const *>(m_cur);

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
  std::enable_if_t<!std::is_trivial_v<T>, ReverseMapVisitor &> operator()(T & val, char const * /* name */)
  {
    val.map(*this);
    return *this;
  }

  template <typename T>
  std::enable_if_t<std::is_trivial_v<T>, ReverseMapVisitor &> operator()(T & val, char const * /* name */)
  {
    T * valPtr = reinterpret_cast<T *>(m_cur);
    *valPtr = ReverseByteOrder(*valPtr);
    val = *valPtr;

    m_cur = Align8Ptr(m_cur + sizeof(T));
    return *this;
  }

  template <typename T>
  ReverseMapVisitor & operator()(succinct::mapper::mappable_vector<T> & vec, char const * /* name */)
  {
    vec.clear();
    (*this)(vec.m_size, "size");

    T * data = reinterpret_cast<T *>(m_cur);
    for (uint64_t i = 0; i < vec.m_size; ++i)
      data[i] = ReverseByteOrder(data[i]);
    vec.m_data = data;

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
  std::enable_if_t<!std::is_trivial_v<T>, FreezeVisitor &> operator()(T & val, char const * /* name */)
  {
    ASSERT(IsAlign8(m_writer.Pos()), ());
    val.map(*this);
    return *this;
  }

  template <typename T>
  std::enable_if_t<std::is_trivial_v<T>, FreezeVisitor &> operator()(T & val, char const * /* name */)
  {
    ASSERT(IsAlign8(m_writer.Pos()), ());
    m_writer.Write(&val, sizeof(T));
    m_bytesWritten += sizeof(T);
    WritePadding(m_writer, m_bytesWritten);
    return *this;
  }

  template <typename T>
  FreezeVisitor & operator()(succinct::mapper::mappable_vector<T> & vec, char const * /* name */)
  {
    ASSERT(IsAlign8(m_writer.Pos()), ());
    (*this)(vec.m_size, "size");

    size_t const bytes = static_cast<size_t>(vec.m_size * sizeof(T));
    m_writer.Write(vec.m_data, bytes);
    m_bytesWritten += bytes;
    WritePadding(m_writer, m_bytesWritten);
    return *this;
  }

  uint64_t BytesWritten() const { return m_bytesWritten; }

private:
  TWriter & m_writer;
  uint64_t m_bytesWritten;

  DISALLOW_COPY_AND_MOVE(FreezeVisitor);
};

template <typename TWriter>
class ReverseFreezeVisitor
{
public:
  explicit ReverseFreezeVisitor(TWriter & writer) : m_writer(writer), m_bytesWritten(0) {}

  template <typename T>
  std::enable_if_t<!std::is_trivial_v<T>, ReverseFreezeVisitor &> operator()(T & val, char const * /* name */)
  {
    ASSERT(IsAlign8(m_writer.Pos()), ());
    val.map(*this);
    return *this;
  }

  template <typename T>
  std::enable_if_t<std::is_trivial_v<T>, ReverseFreezeVisitor &> operator()(T & val, char const * /* name */)
  {
    ASSERT(IsAlign8(m_writer.Pos()), ());
    T const reversedVal = ReverseByteOrder(val);
    m_writer.Write(&reversedVal, sizeof(reversedVal));
    m_bytesWritten += sizeof(T);
    WritePadding(m_writer, m_bytesWritten);
    return *this;
  }

  template <typename T>
  ReverseFreezeVisitor & operator()(succinct::mapper::mappable_vector<T> & vec, char const * /* name */)
  {
    ASSERT(IsAlign8(m_writer.Pos()), ());
    (*this)(vec.m_size, "size");

    for (auto const & val : vec)
    {
      T const reversedVal = ReverseByteOrder(val);
      m_writer.Write(&reversedVal, sizeof(reversedVal));
    }
    m_bytesWritten += static_cast<size_t>(vec.m_size * sizeof(T));
    WritePadding(m_writer, m_bytesWritten);
    return *this;
  }

  uint64_t BytesWritten() const { return m_bytesWritten; }

private:
  TWriter & m_writer;
  uint64_t m_bytesWritten;

  DISALLOW_COPY_AND_MOVE(ReverseFreezeVisitor);
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

template <typename T, typename TWriter>
uint64_t ReverseFreeze(T & val, TWriter & writer, char const * name)
{
  ReverseFreezeVisitor<TWriter> visitor(writer);
  visitor(val, name);
  return visitor.BytesWritten();
}
}  // namespace coding
