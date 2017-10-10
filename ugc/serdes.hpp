#pragma once

#include "ugc/types.hpp"

#include "coding/point_to_integer.hpp"
#include "coding/read_write_utils.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/exception.hpp"

#include <cmath>
#include <cstdint>

namespace ugc
{
DECLARE_EXCEPTION(SerDesException, RootException);
DECLARE_EXCEPTION(BadBlob, SerDesException);

enum class Version : uint8_t
{
  V0,
  Latest = V0
};

template <typename Sink>
class Serializer
{
public:
  Serializer(Sink & sink) : m_sink(sink) {}

  void operator()(uint8_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(uint32_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(uint64_t const d, char const * /* name */ = nullptr) { WriteToSink(m_sink, d); }
  void operator()(std::string const & s, char const * /* name */ = nullptr)
  {
    rw::Write(m_sink, s);
  }

  void VisitRating(float const f, char const * /* name */ = nullptr)
  {
    CHECK_GREATER_OR_EQUAL(f, 0.0, ());
    auto const d = static_cast<uint32_t>(round(f * 10));
    VisitVarUint(d);
  }

  void VisitLang(uint8_t const index, char const * /* name */ = nullptr)
  {
    WriteToSink(m_sink, index);
  }

  template <typename T>
  void VisitVarUint(T const & t, char const * /* name */ = nullptr)
  {
    WriteVarUint(m_sink, t);
  }

  void operator()(TranslationKey const & key, char const * /* name */ = nullptr)
  {
    (*this)(key.m_key);
  }

  void operator()(Time const & t, char const * /* name */ = nullptr)
  {
    VisitVarUint(ToDaysSinceEpoch(t));
  }

  void operator()(Sentiment sentiment, char const * /* name */ = nullptr)
  {
    switch (sentiment)
    {
    case Sentiment::Negative: return (*this)(static_cast<uint8_t>(0));
    case Sentiment::Positive: return (*this)(static_cast<uint8_t>(1));
    }
  }

  template <typename T>
  void operator()(vector<T> const & vs, char const * /* name */ = nullptr)
  {
    VisitVarUint(static_cast<uint32_t>(vs.size()));
    for (auto const & v : vs)
      (*this)(v);
  }

  template <typename R>
  void operator()(R const & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

private:
  Sink & m_sink;
};

template <typename Source>
class DeserializerV0
{
public:
  DeserializerV0(Source & source) : m_source(source) {}

  void operator()(uint8_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }
  void operator()(uint32_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }
  void operator()(uint64_t & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }
  void operator()(std::string & s, char const * /* name */ = nullptr)
  {
    rw::Read(m_source, s);
  }

  void VisitRating(float & f, char const * /* name */ = nullptr)
  {
    auto const d = DesVarUint<uint32_t>();
    f = static_cast<float>(d) / 10;
  }

  void VisitLang(uint8_t & index, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource<uint8_t>(m_source, index);
  }

  template <typename T>
  void VisitVarUint(T & t, char const * /* name */ = nullptr)
  {
    t = ReadVarUint<T, Source>(m_source);
  }

  template <typename T>
  T DesVarUint()
  {
    return ReadVarUint<T, Source>(m_source);
  }

  void operator()(TranslationKey & key, char const * /* name */ = nullptr) { (*this)(key.m_key); }

  void operator()(Time & t, char const * /* name */ = nullptr)
  {
    t = FromDaysSinceEpoch(DesVarUint<uint32_t>());
  }

  void operator()(Sentiment & sentiment, char const * /* name */ = nullptr)
  {
    uint8_t s = 0;
    (*this)(s);
    switch (s)
    {
    case 0: sentiment = Sentiment::Negative; break;
    case 1: sentiment = Sentiment::Positive; break;
    default: CHECK(false, ("Can't parse sentiment from:", static_cast<int>(s))); break;
    }
  }

  template <typename T>
  void operator()(vector<T> & vs, char const * /* name */ = nullptr)
  {
    auto const size = DesVarUint<uint32_t>();
    vs.resize(size);
    for (auto & v : vs)
      (*this)(v);
  }

  template <typename R>
  void operator()(R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

private:
  Source & m_source;
};

template <typename Sink, typename UGC>
void Serialize(Sink & sink, UGC const & ugc)
{
  WriteToSink(sink, static_cast<uint8_t>(Version::Latest));
  Serializer<Sink> ser(sink);
  ser(ugc);
}

template <typename Source, typename UGC>
void Deserialize(Source & source, UGC & ugc)
{
  uint8_t version = 0;
  ReadPrimitiveFromSource(source, version);
  if (version == static_cast<uint8_t>(Version::V0))
  {
    DeserializerV0<Source> des(source);
    des(ugc);
    return;
  }

  MYTHROW(BadBlob, ("Unknown data version:", static_cast<int>(version)));
}
}  // namespace ugc
