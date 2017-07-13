#pragma once

#include "coding/reader.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>

namespace ugc
{
namespace binary
{
namespace impl
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
  HeaderDesVisitor(Source & source): m_source(source) {}

  void operator()(uint64_t & v, char const * /* name */ = nullptr)
  {
    v = ReadPrimitiveFromSource<uint64_t>(m_source);
  }

  template <typename R>
  void operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

  Source & m_source;
};
}  // namespace impl

struct HeaderV0
{
  template <typename Visitor>
  void Visit(Visitor & visitor)
  {
    visitor(m_keysOffset, "keysOffset");
    visitor(m_ugcsOffset, "ugcsOffset");
    visitor(m_indexOffset, "indexOffset");
    visitor(m_textsOffset, "textsOffset");
    visitor(m_eosOffset, "eosOffset");
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    impl::HeaderSerVisitor<Sink> visitor(sink);
    visitor(*this);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    impl::HeaderDesVisitor<Source> visitor(source);
    visitor(*this);
  }

  // Calculates the size of serialized header in bytes.
  uint64_t Size()
  {
    impl::HeaderSizeOfVisitor visitor;
    visitor(*this);
    return visitor.m_size;
  }

  uint64_t m_keysOffset = 0;
  uint64_t m_ugcsOffset = 0;
  uint64_t m_indexOffset = 0;
  uint64_t m_textsOffset = 0;
  uint64_t m_eosOffset = 0;
};
}  // namespace binary
}  // namespace ugc
