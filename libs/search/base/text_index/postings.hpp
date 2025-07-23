#pragma once

#include "search/base/text_index/header.hpp"
#include "search/base/text_index/text_index.hpp"
#include "search/base/text_index/utils.hpp"

#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include <cstdint>
#include <functional>
#include <vector>

namespace search_base
{
struct TextIndexHeader;

// A helper class that fetches the postings lists for
// one token at a time. It is assumed that the tokens
// are enumerated in the lexicographic order.
class PostingsFetcher
{
public:
  using Fn = std::function<void(uint32_t)>;

  virtual ~PostingsFetcher() = default;

  // Returns true when there are tokens left in the fetcher and false otherwise.
  virtual bool IsValid() const = 0;

  // Advances fetcher to the next token.
  virtual void Advance() = 0;

  // Calls |fn| for every posting for the current token. Initially,
  // current token is the first token and then calls to Advance
  // may be used to process the next token until the underlying
  // source of the tokens is exhausted and the fetcher is no longer valid.
  virtual void ForEachPosting(Fn const & fn) const = 0;
};

// Fetches the postings list one by one from |fetcher| and writes them
// to |sink|, updating the fields in |header| that correspond to the
// postings list.
// |startPos| marks the start of the entire text index and is needed to compute
// the offsets that are stored in |header|.
template <typename Sink>
void WritePostings(Sink & sink, uint64_t startPos, TextIndexHeader & header, PostingsFetcher & fetcher)
{
  header.m_postingsStartsOffset = RelativePos(sink, startPos);
  // An uint32_t for each 32-bit offset and an uint32_t for the dummy entry at the end.
  WriteZeroesToSink(sink, sizeof(uint32_t) * (header.m_numTokens + 1));

  header.m_postingsListsOffset = RelativePos(sink, startPos);

  std::vector<uint32_t> postingsStarts;
  postingsStarts.reserve(header.m_numTokens);
  {
    uint32_t last;
    // todo(@m) s/uint32_t/Posting/ ?
    auto writePostings = [&](uint32_t p)
    {
      CHECK(last == 0 || last < p, (last, p));
      uint32_t const delta = p - last;
      WriteVarUint(sink, delta);
      last = p;
    };
    while (fetcher.IsValid())
    {
      postingsStarts.emplace_back(RelativePos(sink, startPos));
      last = 0;
      fetcher.ForEachPosting(writePostings);
      fetcher.Advance();
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
}  // namespace search_base
