#include "testing/testing.hpp"

#include "search/search_query_params.hpp"
#include "search/v2/ranking_utils.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"

using namespace search;
using namespace search::v2;
using namespace strings;

namespace
{
NameScore GetScore(string const & name, string const & query, size_t startToken, size_t endToken)
{
  search::Delimiters delims;
  SearchQueryParams params;
  auto addToken = [&params](UniString const & token)
  {
    params.m_tokens.push_back({token});
  };

  SplitUniString(NormalizeAndSimplifyString(query), addToken, delims);
  if (!params.m_tokens.empty() && !delims(strings::LastUniChar(query)))
  {
    params.m_prefixTokens.swap(params.m_tokens.back());
    params.m_tokens.pop_back();
  }
  return GetNameScore(name, TokensSlice(params, startToken, endToken));
}

UNIT_TEST(NameTest_Smoke)
{
  TEST_EQUAL(GetScore("New York", "Central Park, New York, US", 2, 4), NAME_SCORE_FULL_MATCH, ());
  TEST_EQUAL(GetScore("New York", "York", 0, 1), NAME_SCORE_SUBSTRING, ());
  TEST_EQUAL(GetScore("Moscow", "Red Square Mosc", 2, 3), NAME_SCORE_FULL_MATCH_PREFIX, ());
  TEST_EQUAL(GetScore("Moscow", "Red Square Moscow", 2, 3), NAME_SCORE_FULL_MATCH, ());
  TEST_EQUAL(GetScore("San Francisco", "Fran", 0, 1), NAME_SCORE_SUBSTRING_PREFIX, ());
  TEST_EQUAL(GetScore("San Francisco", "Fran ", 0, 1), NAME_SCORE_ZERO, ());
}
}  // namespace
