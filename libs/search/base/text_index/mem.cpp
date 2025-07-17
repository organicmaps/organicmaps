#include "search/base/text_index/mem.hpp"

#include "base/stl_helpers.hpp"

using namespace std;

namespace search_base
{
void MemTextIndex::AddPosting(Token const & token, Posting const & posting)
{
  m_postingsByToken[token].emplace_back(posting);
}

void MemTextIndex::SortPostings()
{
  for (auto & entry : m_postingsByToken)
  {
    // A posting may occur several times in a document,
    // so we remove duplicates for the docid index.
    // If the count is needed for ranking it may be stored
    // separately.
    base::SortUnique(entry.second);
  }
}

void MemTextIndex::BuildDictionary()
{
  vector<Token> tokens;
  tokens.reserve(m_postingsByToken.size());
  for (auto const & entry : m_postingsByToken)
    tokens.emplace_back(entry.first);
  m_dictionary.SetTokens(std::move(tokens));
}
}  // namespace search_base
