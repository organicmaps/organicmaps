#pragma once

#include "search/base/text_index/dictionary.hpp"
#include "search/base/text_index/header.hpp"
#include "search/base/text_index/postings.hpp"
#include "search/base/text_index/text_index.hpp"
#include "search/base/text_index/utils.hpp"

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
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
  class MemPostingsFetcher : public PostingsFetcher
  {
  public:
    MemPostingsFetcher(std::map<Token, std::vector<Posting>> const & postingsByToken)
    {
      // todo(@m) An unnecessary copy?
      m_postings.reserve(postingsByToken.size());
      for (auto const & entry : postingsByToken)
        m_postings.emplace_back(entry.second);
    }

    // PostingsFetcher overrides:
    bool GetPostingsForNextToken(std::vector<uint32_t> & postings)
    {
      CHECK_LESS_OR_EQUAL(m_tokenId, m_postings.size(), ());
      if (m_tokenId == m_postings.size())
        return false;
      postings.swap(m_postings[m_tokenId++]);
      return true;
    }

  private:
    std::vector<std::vector<uint32_t>> m_postings;
    // Index of the next token to be processed. The
    // copy of the postings list in |m_postings| is not guaranteed
    // to be valid after it's been processed.
    size_t m_tokenId = 0;
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
}  // namespace base
}  // namespace search
