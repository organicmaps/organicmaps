#pragma once
#include "endianness.hpp"
#include "../base/base.hpp"

#include "../base/start_mem_debug.hpp"

template <class TSink> void WriteToSink(TSink & sink, unsigned char c)
{
  sink.Write(&c, 1);
}

template <class TSink> void WriteToSink(TSink & sink, signed char c)
{
  sink.Write(&c, 1);
}

template <class TSink> void WriteToSink(TSink & sink, char c)
{
  sink.Write(&c, 1);
}

template <class TSink> void WriteToSink(TSink & sink, uint16_t v)
{
  uint16_t t = SwapIfBigEndian(v);
  sink.Write(&t, 2);
}

template <class TSink> void WriteToSink(TSink & sink, int32_t v)
{
  int32_t t = SwapIfBigEndian(v);
  sink.Write(&t, 4);
}

template <class TSink> void WriteToSink(TSink & sink, uint32_t v)
{
  uint32_t t = SwapIfBigEndian(v);
  sink.Write(&t, 4);
}

template <class TSink> void WriteToSink(TSink & sink, int64_t v)
{
  int64_t t = SwapIfBigEndian(v);
  sink.Write(&t, 8);
}

template <class TSink> void WriteToSink(TSink & sink, uint64_t v)
{
  uint64_t t = SwapIfBigEndian(v);
  sink.Write(&t, 8);
}

template <typename SinkT>
struct WriterFunctor
{
  SinkT & m_Sink;
  explicit WriterFunctor(SinkT & sink) : m_Sink(sink) {}

  template <typename T>
  inline void operator() (T const & t) const
  {
    m_Sink.Write(&t, sizeof(T));
  }
};

#include "../base/stop_mem_debug.hpp"
