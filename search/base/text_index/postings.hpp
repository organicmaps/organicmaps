#pragma once

#include "search/base/text_index/header.hpp"
#include "search/base/text_index/text_index.hpp"
#include "search/base/text_index/utils.hpp"

#include "coding/varint.hpp"

#include <vector>

namespace search
{
namespace base
{
struct TextIndexHeader;

// A helper class that fetches the postings lists for
// one token at a time. It is assumed that the tokens
// are enumerated in the lexicographic order.
class PostingsFetcher
{
public:
  // Returns true and fills |postings| with the postings list of the next token
  // when there is one.
  // Returns false if the underlying source is exhausted, i.e. there are
  // no more tokens left.
  virtual bool GetPostingsForNextToken(std::vector<uint32_t> & postings) = 0;
};

// Fetches the postings list one by one from |fetcher| and writes them
// to |sink|, updating the fields in |header| that correspond to the
// postings list.
// |startPos| marks the start of the entire text index and is needed to compute
// the offsets that are stored in |header|.
template <typename Sink>
void WritePostings(Sink & sink, uint64_t startPos, TextIndexHeader & header,
                   PostingsFetcher & fetcher)
{
  header.m_postingsStartsOffset = RelativePos(sink, startPos);
  // An uint32_t for each 32-bit offset and an uint32_t for the dummy entry at the end.
  WriteZeroesToSink(sink, sizeof(uint32_t) * (header.m_numTokens + 1));

  header.m_postingsListsOffset = RelativePos(sink, startPos);

  std::vector<uint32_t> postingsStarts;
  postingsStarts.reserve(header.m_numTokens);

  // todo(@m) s/uint32_t/Posting/ ?
  std::vector<uint32_t> postings;
  while (fetcher.GetPostingsForNextToken(postings))
  {
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
}  // namespace base
}  // namespace search
