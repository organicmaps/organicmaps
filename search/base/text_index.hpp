#pragma once

#include "coding/reader.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"
#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

// This file contains the structures needed to store an
// updatable text index on disk.
//
// The index maps tokens of string type (typically std::string or
// strings::UniString) to postings lists, i.e. to lists of entities
// called postings that encode the locations of the strings in the collection
// of the text documents that is being indexed. An example of a posting
// is a document id (docid). Another example is a pair of a document id and
// a position within the corresponding document.
//
// The updates are performed by rebuilding the index, either as a result
// of merging several indexes together, or as a result of clearing outdated
// entries from an old index.
//
// For version 0, the postings lists are docid arrays, i.e. arrays of unsigned
// 32-bit integers stored in increasing order.
// The structure of the index is:
//   [header: version and offsets]
//   [array containing the starting positions of tokens]
//   [tokens, written without separators in the lexicographical order]
//   [array containing the offsets for the postings lists]
//   [postings lists, stored as delta-encoded varints]
//
// All offsets are measured relative to the start of the index.
namespace search
{
namespace base
{
using Posting = uint32_t;

enum class TextIndexVersion : uint8_t
{
  V0 = 0,
  Latest = V0
};

struct TextIndexHeader
{
  template <typename Sink>
  void Serialize(Sink & sink) const
  {
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());

    sink.Write(kHeaderMagic.data(), kHeaderMagic.size());
    WriteToSink(sink, static_cast<uint8_t>(m_version));
    WriteToSink(sink, m_numTokens);
    WriteToSink(sink, m_dictPositionsOffset);
    WriteToSink(sink, m_dictWordsOffset);
    WriteToSink(sink, m_postingsStartsOffset);
    WriteToSink(sink, m_postingsListsOffset);
  }

  template <typename Source>
  void Deserialize(Source & source)
  {
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());

    std::string headerMagic(kHeaderMagic.size(), ' ');
    source.Read(&headerMagic[0], headerMagic.size());
    CHECK_EQUAL(headerMagic, kHeaderMagic, ());
    m_version = static_cast<TextIndexVersion>(ReadPrimitiveFromSource<uint8_t>(source));
    m_numTokens = ReadPrimitiveFromSource<uint32_t>(source);
    m_dictPositionsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_dictWordsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_postingsStartsOffset = ReadPrimitiveFromSource<uint32_t>(source);
    m_postingsListsOffset = ReadPrimitiveFromSource<uint32_t>(source);
  }

  static std::string const kHeaderMagic;
  TextIndexVersion m_version = TextIndexVersion::Latest;
  uint32_t m_numTokens = 0;
  uint32_t m_dictPositionsOffset = 0;
  uint32_t m_dictWordsOffset = 0;
  uint32_t m_postingsStartsOffset = 0;
  uint32_t m_postingsListsOffset = 0;
};

template <typename Token>
class MemTextIndex
{
public:
  MemTextIndex() = default;

  void AddPosting(Token const & token, Posting const & posting)
  {
    m_postingsByToken[token].emplace_back(posting);
  }

  // Executes |f| on every posting associated with |token|.
  // The order of postings is not specified.
  template <typename F>
  void ForEachPosting(Token const & token, F && f) const
  {
    auto it = m_postingsByToken.find(token);
    if (it == m_postingsByToken.end())
      return;
    for (auto const & p : it->second)
      f(p);
  }

  template <typename Sink>
  void Serialize(Sink & sink)
  {
    SortPostings();

    TextIndexHeader header;

    uint64_t const startPos = sink.Pos();
    header.Serialize(sink);

    header.m_numTokens = ::base::checked_cast<uint32_t>(m_postingsByToken.size());

    header.m_dictPositionsOffset = RelativePos(sink, startPos);
    uint32_t offset = header.m_dictPositionsOffset;
    for (auto const & entry : m_postingsByToken)
    {
      auto const & token = entry.first;
      WriteToSink(sink, offset);
      offset += static_cast<uint32_t>(token.size());
    }
    // One more for convenience.
    WriteToSink(sink, offset);

    header.m_dictWordsOffset = RelativePos(sink, startPos);
    for (auto const & entry : m_postingsByToken)
    {
      auto const & token = entry.first;
      sink.Write(token.data(), token.size());
    }

    header.m_postingsStartsOffset = RelativePos(sink, startPos);
    // 4 bytes for each 32-bit position and 4 bytes for the dummy entry at the end.
    WriteZeroesToSink(sink, 4 * (header.m_numTokens + 1));

    header.m_postingsListsOffset = RelativePos(sink, startPos);

    std::vector<uint32_t> postingsStarts;
    postingsStarts.reserve(header.m_numTokens);
    for (auto const & entry : m_postingsByToken)
    {
      auto const & postings = entry.second;

      postingsStarts.emplace_back(RelativePos(sink, startPos));

      uint64_t last = 0;
      for (auto const & p : postings)
      {
        CHECK(last == 0 || last < p, (last, p));
        uint64_t const delta = ::base::checked_cast<uint64_t>(p) - last;
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

    CHECK_EQUAL(source.Pos(), startPos + header.m_dictPositionsOffset, ());
    std::vector<uint32_t> tokenOffsets(header.m_numTokens + 1);
    for (size_t i = 0; i < tokenOffsets.size(); ++i)
      tokenOffsets[i] = ReadPrimitiveFromSource<uint32_t>(source);

    CHECK_EQUAL(source.Pos(), startPos + header.m_dictWordsOffset, ());
    std::vector<Token> tokens(header.m_numTokens);
    for (size_t i = 0; i < tokens.size(); ++i)
    {
      size_t const size = ::base::checked_cast<size_t>(tokenOffsets[i + 1] - tokenOffsets[i]);
      tokens[i].resize(size);
      source.Read(&tokens[i][0], size);
    }

    CHECK_EQUAL(source.Pos(), startPos + header.m_postingsStartsOffset, ());
    std::vector<uint32_t> postingsStarts(header.m_numTokens + 1);
    for (size_t i = 0; i < postingsStarts.size(); ++i)
      postingsStarts[i] = ReadPrimitiveFromSource<uint32_t>(source);

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

private:
  void SortPostings()
  {
    for (auto & entry : m_postingsByToken)
    {
      // A posting may occur several times in a document,
      // so we remove duplicates for the docid index.
      // If the count is needed for ranking it may be stored
      // separately.
      my::SortUnique(entry.second);
    }
  }

  template <typename Sink>
  uint32_t RelativePos(Sink & sink, uint64_t startPos)
  {
    return ::base::checked_cast<uint32_t>(sink.Pos() - startPos);
  }

  std::map<Token, std::vector<Posting>> m_postingsByToken;
};

std::string DebugPrint(TextIndexVersion const & version);
}  // namespace base
}  // namespace search
