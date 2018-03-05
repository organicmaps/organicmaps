#pragma once

#include "coding/endianness.hpp"

#include <type_traits>

template <class TSink, typename T>
std::enable_if_t<std::is_integral<T>::value || std::is_enum<T>::value, void> WriteToSink(
    TSink & sink, T const & v)
{
  T const t = SwapIfBigEndian(v);
  sink.Write(&t, sizeof(T));
}

template <class TSink> void WriteZeroesToSink(TSink & sink, uint64_t size)
{
  uint8_t const zeroes[256] = { 0 };
  for (uint64_t i = 0; i < (size >> 8); ++i)
    sink.Write(zeroes, 256);
  sink.Write(zeroes, size & 255);
}

template <typename SinkT> class WriterFunctor
{
  SinkT & m_Sink;

public:
  explicit WriterFunctor(SinkT & sink) : m_Sink(sink) {}
  template <typename T> void operator() (T const & t) const
  {
    m_Sink.Write(&t, sizeof(T));
  }
};
