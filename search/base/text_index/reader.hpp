#pragma once

#include "search/base/text_index/dictionary.hpp"
#include "search/base/text_index/text_index.hpp"

#include "coding/file_reader.hpp"
#include "coding/reader.hpp"
#include "coding/varint.hpp"

#include "base/assert.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace search_base
{
// A reader class for on-demand reading of postings lists from disk.
class TextIndexReader
{
public:
  explicit TextIndexReader(FileReader const & fileReader) : m_fileReader(fileReader)
  {
    ReaderSource<FileReader> headerSource(m_fileReader);
    TextIndexHeader header;
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

    ReaderSource<FileReader> source(
        m_fileReader.SubReader(m_postingsStarts[tokenId], m_postingsStarts[tokenId + 1] - m_postingsStarts[tokenId]));

    uint32_t last = 0;
    while (source.Size() > 0)
    {
      last += ReadVarUint<uint32_t>(source);
      fn(last);
    }
  }

  template <typename Fn>
  void ForEachPosting(strings::UniString const & token, Fn && fn) const
  {
    auto const utf8s = strings::ToUtf8(token);
    ForEachPosting(std::move(utf8s), std::forward<Fn>(fn));
  }

  TextIndexDictionary const & GetDictionary() const { return m_dictionary; }

private:
  FileReader m_fileReader;
  TextIndexDictionary m_dictionary;
  std::vector<uint32_t> m_postingsStarts;
};
}  // namespace search_base
