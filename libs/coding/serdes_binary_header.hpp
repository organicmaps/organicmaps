#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>

namespace coding
{
namespace binary
{
struct HeaderSizeOfVisitor
{
  void operator()(uint64_t v, char const * /* name */ = nullptr) { m_size += sizeof(v); }

  template <typename R>
  void operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  uint64_t m_size = 0;
};

template <typename Sink>
struct HeaderSerVisitor
{
  HeaderSerVisitor(Sink & sink) : m_sink(sink) {}

  void operator()(uint64_t v, char const * /* name */ = nullptr) { WriteToSink(m_sink, v); }

  template <typename R>
  void operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  Sink & m_sink;
};

template <typename Source>
struct HeaderDesVisitor
{
  HeaderDesVisitor(Source & source) : m_source(source) {}

  void operator()(uint64_t & v, char const * /* name */ = nullptr) { v = ReadPrimitiveFromSource<uint64_t>(m_source); }

  template <typename R>
  void operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  Source & m_source;
};
}  // namespace binary
}  // namespace coding
