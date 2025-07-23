#include "search/base/text_index/merger.hpp"

#include "search/base/text_index/dictionary.hpp"
#include "search/base/text_index/header.hpp"
#include "search/base/text_index/postings.hpp"

#include "coding/file_writer.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <utility>
#include <vector>

using namespace std;

namespace
{
using namespace search_base;

class MergedPostingsListFetcher : public PostingsFetcher
{
public:
  MergedPostingsListFetcher(TextIndexDictionary const & dict, TextIndexReader const & index1,
                            TextIndexReader const & index2)
    : m_dict(dict)
    , m_index1(index1)
    , m_index2(index2)
  {
    ReadPostings();
  }

  // PostingsFetcher overrides:
  bool IsValid() const override
  {
    auto const & tokens = m_dict.GetTokens();
    CHECK_LESS_OR_EQUAL(m_tokenId, tokens.size(), ());
    return m_tokenId < tokens.size();
  }

  void Advance() override
  {
    auto const & tokens = m_dict.GetTokens();
    CHECK_LESS_OR_EQUAL(m_tokenId, tokens.size(), ());
    if (m_tokenId == tokens.size())
      return;

    ++m_tokenId;
    ReadPostings();
  }

  void ForEachPosting(Fn const & fn) const override
  {
    CHECK(IsValid(), ());
    for (uint32_t p : m_postings)
      fn(p);
  }

private:
  // Reads postings for the current token.
  void ReadPostings()
  {
    m_postings.clear();
    if (!IsValid())
      return;

    auto const & tokens = m_dict.GetTokens();
    m_index1.ForEachPosting(tokens[m_tokenId], base::MakeBackInsertFunctor(m_postings));
    m_index2.ForEachPosting(tokens[m_tokenId], base::MakeBackInsertFunctor(m_postings));
    base::SortUnique(m_postings);
  }

  TextIndexDictionary const & m_dict;
  TextIndexReader const & m_index1;
  TextIndexReader const & m_index2;
  // Index of the next token from |m_dict| to be processed.
  size_t m_tokenId = 0;
  vector<uint32_t> m_postings;
};

TextIndexDictionary MergeDictionaries(TextIndexDictionary const & dict1, TextIndexDictionary const & dict2)
{
  vector<Token> commonTokens;
  auto const & ts1 = dict1.GetTokens();
  auto const & ts2 = dict2.GetTokens();
  merge(ts1.begin(), ts1.end(), ts2.begin(), ts2.end(), back_inserter(commonTokens));
  ASSERT(is_sorted(commonTokens.begin(), commonTokens.end()), ());
  commonTokens.erase(unique(commonTokens.begin(), commonTokens.end()), commonTokens.end());

  TextIndexDictionary dict;
  dict.SetTokens(std::move(commonTokens));
  return dict;
}
}  // namespace

namespace search_base
{
// static
void TextIndexMerger::Merge(TextIndexReader const & index1, TextIndexReader const & index2, FileWriter & sink)
{
  TextIndexDictionary const dict = MergeDictionaries(index1.GetDictionary(), index2.GetDictionary());

  TextIndexHeader header;

  uint64_t const startPos = sink.Pos();
  // Will be filled in later.
  header.Serialize(sink);

  dict.Serialize(sink, header, startPos);

  MergedPostingsListFetcher fetcher(dict, index1, index2);
  WritePostings(sink, startPos, header, fetcher);

  // Fill in the header.
  uint64_t const finishPos = sink.Pos();
  sink.Seek(startPos);
  header.Serialize(sink);
  sink.Seek(finishPos);
}
}  // namespace search_base
