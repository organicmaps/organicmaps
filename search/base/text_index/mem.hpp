#pragma once

#include "search/base/text_index/dictionary.hpp"
#include "search/base/text_index/header.hpp"
#include "search/base/text_index/postings.hpp"
#include "search/base/text_index/text_index.hpp"
#include "search/base/text_index/utils.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace search_base
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
    ForEachPosting(strings::ToUtf8(token), std::forward<Fn>(fn));
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
  class MemPostingsFetcher : public PostingsFetcher
  {
  public:
    explicit MemPostingsFetcher(std::map<Token, std::vector<Posting>> const & postingsByToken)
      : m_postingsByToken(postingsByToken)
      , m_it(m_postingsByToken.begin())
    {}

    // PostingsFetcher overrides:
    bool IsValid() const override { return m_it != m_postingsByToken.end(); }

    void Advance() override
    {
      if (m_it != m_postingsByToken.end())
        ++m_it;
    }

    void ForEachPosting(Fn const & fn) const override
    {
      CHECK(IsValid(), ());
      for (uint32_t p : m_it->second)
        fn(p);
    }

  private:
    std::map<Token, std::vector<Posting>> const & m_postingsByToken;
    // Iterator to the current token that will be processed when ForEachPosting is called.
    std::map<Token, std::vector<Posting>>::const_iterator m_it;
  };

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
    MemPostingsFetcher fetcher(m_postingsByToken);
    WritePostings(sink, startPos, header, fetcher);
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

  std::map<Token, std::vector<Posting>> m_postingsByToken;
  TextIndexDictionary m_dictionary;
};
}  // namespace search_base
