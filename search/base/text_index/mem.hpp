#pragma once

#include "search/base/text_index/dictionary.hpp"
#include "search/base/text_index/header.hpp"
#include "search/base/text_index/text_index.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace search
{
namespace base
{
class MemTextIndex
{
public:
  MemTextIndex() = default;

  void AddPosting(Token const & token, Posting const & posting);

  // Executes |fn| on every posting associated with |token|.
  // The order of postings is not specified.
  template <typename Fn>
  void ForEachPosting(Token const & token, Fn && fn) const
  {
    auto const it = m_postingsByToken.find(token);
    if (it == m_postingsByToken.end())
      return;
    for (auto const p : it->second)
      fn(p);
  }

  template <typename Fn>
  void ForEachPosting(strings::UniString const & token, Fn && fn) const
  {
    auto const utf8s = strings::ToUtf8(token);
    ForEachPosting(std::move(utf8s), std::forward<Fn>(fn));
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    SortPostings();
    BuildDictionary();

    TextIndexHeader header;

    uint64_t const startPos = sink.Pos();
    // Will be filled in later.
    header.Serialize(sink);

    SerializeDictionary(sink, header, startPos);
    SerializePostingsLists(sink, header, startPos);

    uint64_t const finishPos = sink.Pos();
    sink.Seek(startPos);
    header.Serialize(sink);
    sink.Seek(finishPos);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    uint64_t startPos = source.Pos();

    TextIndexHeader header;
    header.Deserialize(source);

    DeserializeDictionary(source, header, startPos);
    DeserializePostingsLists(source, header, startPos);
  }

private:
  void SortPostings();

  void BuildDictionary();

  template <typename Sink>
  void SerializeDictionary(Sink & sink, TextIndexHeader & header, uint64_t startPos) const
  {
    m_dictionary.Serialize(sink, header, startPos);
  }

  template <typename Source>
  void DeserializeDictionary(Source & source, TextIndexHeader const & header, uint64_t startPos)
  {
    CHECK_EQUAL(source.Pos(), startPos + header.m_dictPositionsOffset, ());
    m_dictionary.Deserialize(source, header);
  }

  template <typename Sink>
  void SerializePostingsLists(Sink & sink, TextIndexHeader & header, uint64_t startPos) const
  {
    header.m_postingsStartsOffset = RelativePos(sink, startPos);
    // An uint32_t for each 32-bit offset and an uint32_t for the dummy entry at the end.
    WriteZeroesToSink(sink, sizeof(uint32_t) * (header.m_numTokens + 1));

    header.m_postingsListsOffset = RelativePos(sink, startPos);

    std::vector<uint32_t> postingsStarts;
    postingsStarts.reserve(header.m_numTokens);
    for (auto const & entry : m_postingsByToken)
    {
      auto const & postings = entry.second;

      postingsStarts.emplace_back(RelativePos(sink, startPos));

      uint32_t last = 0;
      for (auto const p : postings)
      {
        CHECK(last == 0 || last < p, (last, p));
        uint32_t const delta = p - last;
        WriteVarUint(sink, delta);
        last = p;
      }
    }
    // One more for convenience.
    postingsStarts.emplace_back(RelativePos(sink, startPos));

    {
      uint64_t const savedPos = sink.Pos();
      sink.Seek(startPos + header.m_postingsStartsOffset);
      for (uint32_t const s : postingsStarts)
        WriteToSink(sink, s);

      CHECK_EQUAL(sink.Pos(), startPos + header.m_postingsListsOffset, ());
      sink.Seek(savedPos);
    }
  }

  template <typename Source>
  void DeserializePostingsLists(Source & source, TextIndexHeader const & header, uint64_t startPos)
  {
    CHECK_EQUAL(source.Pos(), startPos + header.m_postingsStartsOffset, ());
    std::vector<uint32_t> postingsStarts(header.m_numTokens + 1);
    for (uint32_t & start : postingsStarts)
      start = ReadPrimitiveFromSource<uint32_t>(source);

    auto const & tokens = m_dictionary.GetTokens();
    CHECK_EQUAL(source.Pos(), startPos + header.m_postingsListsOffset, ());
    m_postingsByToken.clear();
    for (size_t i = 0; i < header.m_numTokens; ++i)
    {
      std::vector<uint32_t> postings;
      uint32_t last = 0;
      while (source.Pos() < startPos + postingsStarts[i + 1])
      {
        last += ReadVarUint<uint32_t>(source);
        postings.emplace_back(last);
      }
      CHECK_EQUAL(source.Pos(), postingsStarts[i + 1], ());

      m_postingsByToken.emplace(tokens[i], postings);
    }
  }

  template <typename Sink>
  static uint32_t RelativePos(Sink & sink, uint64_t startPos)
  {
    return ::base::checked_cast<uint32_t>(sink.Pos() - startPos);
  }

  std::map<Token, std::vector<Posting>> m_postingsByToken;
  TextIndexDictionary m_dictionary;
};
}  // namespace base
}  // namespace search
