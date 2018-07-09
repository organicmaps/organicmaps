#include "testing/testing.hpp"

#include "search/query_params.hpp"
#include "search/ranking_utils.hpp"
#include "search/token_range.hpp"
#include "search/token_slice.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include "std/cstdint.hpp"
#include "std/string.hpp"

using namespace search;
using namespace strings;

namespace
{
NameScore GetScore(string const & name, string const & query, TokenRange const & tokenRange)
{
  search::Delimiters delims;
  QueryParams params;

  vector<UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(query), MakeBackInsertFunctor(tokens), delims);

  if (!query.empty() && !delims(strings::LastUniChar(query)))
  {
    CHECK(!tokens.empty(), ());
    params.InitWithPrefix(tokens.begin(), tokens.end() - 1, tokens.back());
  }
  else
  {
    params.InitNoPrefix(tokens.begin(), tokens.end());
  }

  return GetNameScore(name, TokenSlice(params, tokenRange));
}

UNIT_TEST(NameTest_Smoke)
{
  TEST_EQUAL(GetScore("New York", "Central Park, New York, US", TokenRange(2, 4)),
             NAME_SCORE_FULL_MATCH, ());
  TEST_EQUAL(GetScore("New York", "York", TokenRange(0, 1)), NAME_SCORE_SUBSTRING, ());
  TEST_EQUAL(GetScore("Moscow", "Red Square Mosc", TokenRange(2, 3)), NAME_SCORE_PREFIX, ());
  TEST_EQUAL(GetScore("Moscow", "Red Square Moscow", TokenRange(2, 3)), NAME_SCORE_FULL_MATCH, ());
  TEST_EQUAL(GetScore("San Francisco", "Fran", TokenRange(0, 1)), NAME_SCORE_SUBSTRING, ());
  TEST_EQUAL(GetScore("San Francisco", "Fran ", TokenRange(0, 1)), NAME_SCORE_ZERO, ());
  TEST_EQUAL(GetScore("San Francisco", "Sa", TokenRange(0, 1)), NAME_SCORE_PREFIX, ());
  TEST_EQUAL(GetScore("San Francisco", "San ", TokenRange(0, 1)), NAME_SCORE_PREFIX, ());
  TEST_EQUAL(GetScore("Лермонтовъ", "Лермонтов", TokenRange(0, 1)), NAME_SCORE_PREFIX, ());
  TEST_EQUAL(GetScore("фото на документы", "фото", TokenRange(0, 1)), NAME_SCORE_PREFIX, ());
  TEST_EQUAL(GetScore("фотоателье", "фото", TokenRange(0, 1)), NAME_SCORE_PREFIX, ());
}
}  // namespace
