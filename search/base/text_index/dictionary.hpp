#pragma once

#include "search/base/text_index/header.hpp"
#include "search/base/text_index/text_index.hpp"

#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/checked_cast.hpp"

#include <algorithm>
#include <cstdint>
#include <utility>
#include <vector>

namespace search_base
{
// The dictionary contains all tokens that are present
// in the text index.
class TextIndexDictionary
{
public:
  bool GetTokenId(Token const & token, size_t & id) const
  {
    auto const it = std::lower_bound(m_tokens.cbegin(), m_tokens.cend(), token);
    if (it == m_tokens.cend() || *it != token)
      return false;
    id = base::checked_cast<uint32_t>(std::distance(m_tokens.cbegin(), it));
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
    header.m_numTokens = base::checked_cast<uint32_t>(m_tokens.size());

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
      size_t const size = base::checked_cast<size_t>(tokenOffsets[i + 1] - tokenOffsets[i]);
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
    return base::checked_cast<uint32_t>(sink.Pos() - startPos);
  }

  std::vector<Token> m_tokens;
};
}  // namespace search_base
