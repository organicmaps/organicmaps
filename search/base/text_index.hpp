#pragma once

#include "coding/file_reader.hpp"
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
    CHECK_EQUAL(m_version, TextIndexVersion::V0, ());
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

// The dictionary contains all tokens that are present
// in the text index.
template <typename Token>
class TextIndexDictionary
{
public:
  bool GetTokenId(Token const & token, size_t & id) const
  {
    auto const it = std::lower_bound(m_tokens.cbegin(), m_tokens.cend(), token);
    if (it == m_tokens.cend() || *it != token)
      return false;
    id = ::base::checked_cast<size_t>(std::distance(m_tokens.cbegin(), it));
    return true;
  }

  void SetTokens(std::vector<Token> && tokens)
  {
    ASSERT(std::is_sorted(tokens.begin(), tokens.end()), ());
    m_tokens = std::move(tokens);
  }
  std::vector<Token> const & GetTokens() const { return m_tokens; }

  template <typename Sink>
  void Serialize(Sink & sink, TextIndexHeader & header, uint64_t startPos) const
  {
    header.m_numTokens = ::base::checked_cast<uint32_t>(m_tokens.size());

    header.m_dictPositionsOffset = RelativePos(sink, startPos);
    // An uint32_t for each 32-bit offset and an uint32_t for the dummy entry at the end.
    WriteZeroesToSink(sink, sizeof(uint32_t) * (header.m_numTokens + 1));
    header.m_dictWordsOffset = RelativePos(sink, startPos);

    std::vector<uint32_t> offsets;
    offsets.reserve(header.m_numTokens + 1);
    for (auto const & token : m_tokens)
    {
      offsets.emplace_back(RelativePos(sink, startPos));
      SerializeToken(sink, token);
    }
    offsets.emplace_back(RelativePos(sink, startPos));

    {
      uint64_t const savedPos = sink.Pos();
      sink.Seek(startPos + header.m_dictPositionsOffset);

      for (uint32_t const o : offsets)
        WriteToSink(sink, o);

      CHECK_EQUAL(sink.Pos(), startPos + header.m_dictWordsOffset, ());
      sink.Seek(savedPos);
    }
  }

  template <typename Source>
  void Deserialize(Source & source, TextIndexHeader const & header)
  {
    auto const startPos = source.Pos();

    std::vector<uint32_t> tokenOffsets(header.m_numTokens + 1);
    for (uint32_t & offset : tokenOffsets)
      offset = ReadPrimitiveFromSource<uint32_t>(source);

    uint64_t const expectedSize = header.m_dictWordsOffset - header.m_dictPositionsOffset;
    CHECK_EQUAL(source.Pos(), startPos + expectedSize, ());
    m_tokens.resize(header.m_numTokens);
    for (size_t i = 0; i < m_tokens.size(); ++i)
    {
      size_t const size = ::base::checked_cast<size_t>(tokenOffsets[i + 1] - tokenOffsets[i]);
      DeserializeToken(source, m_tokens[i], size);
    }
  }

private:
  template <typename Sink>
  static void SerializeToken(Sink & sink, Token const & token)
  {
    CHECK(!token.empty(), ());
    // todo(@m) Endianness.
    sink.Write(token.data(), token.size() * sizeof(typename Token::value_type));
  }

  template <typename Source>
  static void DeserializeToken(Source & source, Token & token, size_t size)
  {
    CHECK_GREATER(size, 0, ());
    ASSERT_EQUAL(size % sizeof(typename Token::value_type), 0, ());
    token.resize(size / sizeof(typename Token::value_type));
    source.Read(&token[0], size);
  }

  template <typename Sink>
  static uint32_t RelativePos(Sink & sink, uint64_t startPos)
  {
    return ::base::checked_cast<uint32_t>(sink.Pos() - startPos);
  }

  std::vector<Token> m_tokens;
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

  void BuildDictionary()
  {
    std::vector<Token> tokens;
    tokens.reserve(m_postingsByToken.size());
    for (auto const & entry : m_postingsByToken)
      tokens.emplace_back(entry.first);
    m_dictionary.SetTokens(std::move(tokens));
  }

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
  TextIndexDictionary<Token> m_dictionary;
};

// A reader class for on-demand reading of postings lists from disk.
template <typename Token>
class TextIndexReader
{
public:
  explicit TextIndexReader(FileReader const & fileReader) : m_fileReader(fileReader)
  {
    TextIndexHeader header;
    ReaderSource<FileReader> headerSource(m_fileReader);
    header.Deserialize(headerSource);

    uint64_t const dictStart = header.m_dictPositionsOffset;
    uint64_t const dictEnd = header.m_postingsStartsOffset;
    ReaderSource<FileReader> dictSource(m_fileReader.SubReader(dictStart, dictEnd - dictStart));
    m_dictionary.Deserialize(dictSource, header);

    uint64_t const postStart = header.m_postingsStartsOffset;
    uint64_t const postEnd = header.m_postingsListsOffset;
    ReaderSource<FileReader> postingsSource(m_fileReader.SubReader(postStart, postEnd - postStart));
    m_postingsStarts.resize(header.m_numTokens + 1);
    for (uint32_t & start : m_postingsStarts)
      start = ReadPrimitiveFromSource<uint32_t>(postingsSource);
  }

  // Executes |fn| on every posting associated with |token|.
  // The order of postings is not specified.
  template <typename Fn>
  void ForEachPosting(Token const & token, Fn && fn) const
  {
    size_t tokenId = 0;
    if (!m_dictionary.GetTokenId(token, tokenId))
      return;
    CHECK_LESS(tokenId + 1, m_postingsStarts.size(), ());

    ReaderSource<FileReader> source(m_fileReader.SubReader(
        m_postingsStarts[tokenId], m_postingsStarts[tokenId + 1] - m_postingsStarts[tokenId]));

    uint32_t last = 0;
    while (source.Size() > 0)
    {
      last += ReadVarUint<uint32_t>(source);
      fn(last);
    }
  }

private:
  FileReader m_fileReader;
  TextIndexDictionary<Token> m_dictionary;
  std::vector<uint32_t> m_postingsStarts;
};

std::string DebugPrint(TextIndexVersion const & version);
}  // namespace base
}  // namespace search
