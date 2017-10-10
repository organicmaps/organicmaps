#pragma once

#include "ugc/types.hpp"

#include "coding/read_write_utils.hpp"
#include "coding/text_storage.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iterator>
#include <string>
#include <type_traits>
#include <vector>

class Reader;

namespace ugc
{
namespace binary
{
// This class is very similar to ugc::Serializer, with a few differences:
// * it writes indices of TranslationKeys instead of writing them directly
// * it writes indices of Texts instead of writing them directly
template <typename Sink>
class SerializerVisitor
{
public:
  // We assume that all texts from the UGC span the contiguous
  // subsequence in the sequence of all texts, and this subsequence
  // starts at |textsFrom|.
  SerializerVisitor(Sink & sink, std::vector<TranslationKey> const & keys,
                    std::vector<Text> const & texts, uint64_t textsFrom)
    : m_sink(sink), m_keys(keys), m_texts(texts), m_textsFrom(textsFrom)
  {
    VisitVarUint(m_textsFrom, "textsFrom");
  }

  void operator()(std::string const & s, char const * /* name */ = nullptr)
  {
    rw::Write(m_sink, s);
  }

  void VisitRating(float const f, char const * name = nullptr)
  {
    CHECK_GREATER_OR_EQUAL(f, 0.0, ());
    auto const d = static_cast<uint32_t>(round(f * 10));
    VisitVarUint(d, name);
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

  void operator()(TranslationKey const & key, char const * name = nullptr)
  {
    auto const it = std::lower_bound(m_keys.begin(), m_keys.end(), key);
    CHECK(it != m_keys.end(), ());
    auto const offset = static_cast<uint64_t>(std::distance(m_keys.begin(), it));
    VisitVarUint(offset, name);
  }

  void operator()(Text const & text, char const * name = nullptr) { (*this)(text.m_lang, "lang"); }

  void operator()(Time const & t, char const * name = nullptr)
  {
    VisitVarUint(ToDaysSinceEpoch(t), name);
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

  template <typename D>
  typename std::enable_if<std::is_integral<D>::value>::type operator()(
      D d, char const * /* name */ = nullptr)
  {
    WriteToSink(m_sink, d);
  }

  template <typename R>
  typename std::enable_if<!std::is_integral<R>::value>::type operator()(
      R const & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

private:
  Sink & m_sink;
  std::vector<TranslationKey> const & m_keys;
  vector<Text> const & m_texts;
  uint64_t m_textsFrom = 0;
};

template <typename Source>
class DeserializerVisitorV0
{
public:
  // |source| must be set to the beginning of the UGC blob.
  // |textsReader| must be set to the blocked text storage section.
  DeserializerVisitorV0(Source & source, std::vector<TranslationKey> const & keys,
                        Reader & textsReader, coding::BlockedTextStorageReader & texts)
    : m_source(source), m_keys(keys), m_textsReader(textsReader), m_texts(texts)
  {
    m_currText = DesVarUint<uint64_t>();
  }

  void operator()(std::string & s, char const * /* name */ = nullptr) { rw::Read(m_source, s); }

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

  void operator()(TranslationKey & key, char const * /* name */ = nullptr)
  {
    auto const index = DesVarUint<uint64_t>();
    CHECK_LESS(index, m_keys.size(), ());
    key = m_keys[static_cast<size_t>(index)];
  }

  void operator()(Text & text, char const * /* name */ = nullptr)
  {
    (*this)(text.m_lang, "lang");
    text.m_text = m_texts.ExtractString(m_textsReader, m_currText);
    ++m_currText;
  }

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
  void operator()(std::vector<T> & vs, char const * /* name */ = nullptr)
  {
    auto const size = DesVarUint<uint32_t>();
    vs.resize(size);
    for (auto & v : vs)
      (*this)(v);
  }

  template <typename D>
  typename std::enable_if<std::is_integral<D>::value>::type operator()(
      D & d, char const * /* name */ = nullptr)
  {
    ReadPrimitiveFromSource(m_source, d);
  }

  template <typename R>
  typename std::enable_if<!std::is_integral<R>::value>::type operator()(
      R & r, char const * /* name */ = nullptr)
  {
    r.Visit(*this);
  }

private:
  Source & m_source;
  std::vector<TranslationKey> const & m_keys;

  Reader & m_textsReader;
  coding::BlockedTextStorageReader & m_texts;

  uint64_t m_currText = 0;
};
}  // namespace binary
}  // namespace ugc
