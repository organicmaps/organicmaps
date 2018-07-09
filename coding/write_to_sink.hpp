#pragma once

#include "coding/endianness.hpp"

#include <cstdint>
#include <type_traits>

template <class Sink, typename T>
std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, void> WriteToSink(
    Sink & sink, T const & v)
{
  T const t = SwapIfBigEndian(v);
  sink.Write(&t, sizeof(T));
}

template <class Sink>
void WriteZeroesToSink(Sink & sink, uint64_t size)
{
  uint8_t const zeroes[256] = { 0 };
  for (uint64_t i = 0; i < (size >> 8); ++i)
    sink.Write(zeroes, 256);
  sink.Write(zeroes, size & 255);
}

template <typename Sink>
class WriterFunctor
{
public:
  explicit WriterFunctor(Sink & sink) : m_sink(sink) {}

  template <typename T> void operator() (T const & t) const
  {
    m_sink.Write(&t, sizeof(T));
  }

private:
  Sink & m_sink;
};
