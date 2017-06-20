#pragma once

#include "ugc/types.hpp"

#include "coding/multilang_utf8_string.hpp"
#include "coding/point_to_integer.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include <cmath>
#include <cstdint>

namespace ugc
{
enum class Version : uint8_t
{
  V0
};

struct HeaderV0
{
};

template <typename Sink>
class Serializer
{
public:
  Serializer(Sink & sink, HeaderV0 const & header) : m_sink(sink), m_header(header) {}

  void operator()(uint8_t const d) { WriteToSink(m_sink, d); }
  void operator()(uint32_t const d) { WriteToSink(m_sink, d); }
  void operator()(uint64_t const d) { WriteToSink(m_sink, d); }
  void operator()(std::string const & s) { utils::WriteString(m_sink, s); }

  void SerRating(float const f)
  {
    CHECK_GREATER_OR_EQUAL(f, 0.0, ());
    auto const d = static_cast<uint32_t>(round(f * 10));
    SerVarUint(d);
  }

  template <typename T>
  void SerVarUint(T const & t)
  {
    WriteVarUint(m_sink, t);
  }

  template <typename T>
  void operator()(vector<T> const & vs)
  {
    SerVarUint(static_cast<uint32_t>(vs.size()));
    for (auto const & v : vs)
      (*this)(v);
  }

  void operator()(RatingRecord const & ratingRecord)
  {
    (*this)(ratingRecord.m_key);
    SerRating(ratingRecord.m_value);
  }

  void operator()(Rating const & rating)
  {
    (*this)(rating.m_ratings);
    SerRating(rating.m_aggValue);
  }

  void operator()(UID const & uid)
  {
    (*this)(uid.m_hi);
    (*this)(uid.m_lo);
  }

  void operator()(Author const & author)
  {
    (*this)(author.m_uid);
    (*this)(author.m_name);
  }

  void operator()(Text const & text)
  {
    (*this)(text.m_lang);
    (*this)(text.m_text);
  }

  void operator()(Sentiment sentiment)
  {
    switch (sentiment)
    {
    case Sentiment::Negative: return (*this)(static_cast<uint8_t>(0));
    case Sentiment::Positive: return (*this)(static_cast<uint8_t>(1));
    }
  }

  void operator()(Review const & review)
  {
    (*this)(review.m_id);
    (*this)(review.m_text);
    (*this)(review.m_author);
    SerRating(review.m_rating);
    (*this)(review.m_sentiment);
    SerVarUint(review.DaysSinceEpoch());
  }

  void operator()(Attribute const & attribute)
  {
    (*this)(attribute.m_key);
    (*this)(attribute.m_value);
  }

  void operator()(UGC const & ugc)
  {
    (*this)(ugc.m_rating);
    (*this)(ugc.m_reviews);
    (*this)(ugc.m_attributes);
  }

private:
  Sink & m_sink;
  HeaderV0 const m_header;
};

template <typename Source>
class DeserializerV0
{
public:
  DeserializerV0(Source & source, HeaderV0 const & header) : m_source(source), m_header(header) {}

  void operator()(uint8_t & d) { ReadPrimitiveFromSource(m_source, d); }
  void operator()(uint32_t & d) { ReadPrimitiveFromSource(m_source, d); }
  void operator()(uint64_t & d) { ReadPrimitiveFromSource(m_source, d); }
  void operator()(std::string & s) { utils::ReadString(m_source, s); }

  void DesRating(float & f)
  {
    uint32_t d = 0;
    DesVarUint(d);
    f = static_cast<float>(d) / 10;
  }

  template <typename T>
  void DesVarUint(T & t)
  {
    t = ReadVarUint<T, Source>(m_source);
  }

  template <typename T>
  T DesVarUint()
  {
    return ReadVarUint<T, Source>(m_source);
  }

  void operator()(RatingRecord & ratingRecord)
  {
    (*this)(ratingRecord.m_key);
    DesRating(ratingRecord.m_value);
  }

  template <typename T>
  void operator()(vector<T> & vs)
  {
    auto const size = DesVarUint<uint32_t>();
    vs.resize(size);
    for (auto & v : vs)
      (*this)(v);
  }

  void operator()(Rating & rating)
  {
    (*this)(rating.m_ratings);
    DesRating(rating.m_aggValue);
  }

  void operator()(UID & uid)
  {
    (*this)(uid.m_hi);
    (*this)(uid.m_lo);
  }

  void operator()(Author & author)
  {
    (*this)(author.m_uid);
    (*this)(author.m_name);
  }

  void operator()(Text & text)
  {
    (*this)(text.m_lang);
    (*this)(text.m_text);
  }

  void operator()(Sentiment & sentiment)
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

  void operator()(Review & review)
  {
    (*this)(review.m_id);
    (*this)(review.m_text);
    (*this)(review.m_author);
    DesRating(review.m_rating);
    (*this)(review.m_sentiment);
    review.SetDaysSinceEpoch(DesVarUint<uint32_t>());
  }

  void operator()(Attribute & attribute)
  {
    (*this)(attribute.m_key);
    (*this)(attribute.m_value);
  }

  void operator()(UGC & ugc)
  {
    (*this)(ugc.m_rating);
    (*this)(ugc.m_reviews);
    (*this)(ugc.m_attributes);
  }

private:
  Source & m_source;
  HeaderV0 const m_header;
};
}  // namespace ugc
