#include "search/base/text_index/merger.hpp"

#include "search/base/text_index/dictionary.hpp"
#include "search/base/text_index/header.hpp"
#include "search/base/text_index/postings.hpp"

#include "coding/file_writer.hpp"
#include "coding/varint.hpp"
#include "coding/write_to_sink.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"
#include "base/stl_add.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <utility>
#include <vector>

using namespace std;

namespace
{
using namespace search::base;

class MergedPostingsListFetcher : public PostingsFetcher
{
public:
  MergedPostingsListFetcher(TextIndexDictionary const & dict, TextIndexReader const & index1,
                            TextIndexReader const & index2)
    : m_dict(dict), m_index1(index1), m_index2(index2)
  {
  }

  // PostingsFetcher overrides:
  bool GetPostingsForNextToken(std::vector<uint32_t> & postings)
  {
    postings.clear();

    auto const & tokens = m_dict.GetTokens();
    CHECK_LESS_OR_EQUAL(m_tokenId, tokens.size(), ());
    if (m_tokenId == tokens.size())
      return false;

    m_index1.ForEachPosting(tokens[m_tokenId], MakeBackInsertFunctor(postings));
    m_index2.ForEachPosting(tokens[m_tokenId], MakeBackInsertFunctor(postings));
    my::SortUnique(postings);
    ++m_tokenId;
    return true;
  }

private:
  TextIndexDictionary const & m_dict;
  TextIndexReader const & m_index1;
  TextIndexReader const & m_index2;
  // Index of the next token from |m_dict| to be processed.
  size_t m_tokenId = 0;
};

TextIndexDictionary MergeDictionaries(TextIndexDictionary const & dict1,
                                      TextIndexDictionary const & dict2)
{
  vector<Token> commonTokens = dict1.GetTokens();
  for (auto const & token : dict2.GetTokens())
  {
    size_t dummy;
    if (!dict1.GetTokenId(token, dummy))
      commonTokens.emplace_back(token);
  }

  sort(commonTokens.begin(), commonTokens.end());
  TextIndexDictionary dict;
  dict.SetTokens(move(commonTokens));
  return dict;
}
}  // namespace

namespace search
{
namespace base
{
// static
void TextIndexMerger::Merge(TextIndexReader const & index1, TextIndexReader const & index2,
                            FileWriter & sink)
{
  TextIndexDictionary const dict =
      MergeDictionaries(index1.GetDictionary(), index2.GetDictionary());

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
}  // namespace base
}  // namespace search
