#include "testing/testing.hpp"

#include "search/query_params.hpp"
#include "search/ranking_utils.hpp"
#include "search/ranking_info.hpp"
#include "search/token_range.hpp"
#include "search/token_slice.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_helpers.hpp"
#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

using namespace search;
using namespace std;
using namespace strings;

namespace
{
NameScores GetScore(string const & name, string const & query, TokenRange const & tokenRange)
{
  search::Delimiters delims;
  QueryParams params;

  vector<UniString> tokens;
  SplitUniString(NormalizeAndSimplifyString(query), base::MakeBackInsertFunctor(tokens), delims);

  if (!query.empty() && !delims(strings::LastUniChar(query)))
  {
    CHECK(!tokens.empty(), ());
    params.InitWithPrefix(tokens.begin(), tokens.end() - 1, tokens.back());
  }
  else
  {
    params.InitNoPrefix(tokens.begin(), tokens.end());
  }

  return GetNameScores(name, StringUtf8Multilang::kDefaultCode, TokenSlice(params, tokenRange));
}

UNIT_TEST(NameTest_Smoke)
{
  auto const test = [](string const & name, string const & query, TokenRange const & tokenRange,
                       NameScore nameScore, size_t errorsMade) {
    TEST_EQUAL(
        GetScore(name, query, tokenRange),
        NameScores(nameScore, nameScore == NAME_SCORE_ZERO ? ErrorsMade() : ErrorsMade(errorsMade),
                   false /* isAltOrOldNAme */),
        (name, query, tokenRange));
  };

  test("New York", "Central Park, New York, US", TokenRange(2, 4), NAME_SCORE_FULL_MATCH, 0);
  test("New York", "York", TokenRange(0, 1), NAME_SCORE_SUBSTRING, 0);
  test("Moscow", "Red Square Mosc", TokenRange(2, 3), NAME_SCORE_PREFIX, 0);
  test("Moscow", "Red Square Moscow", TokenRange(2, 3), NAME_SCORE_FULL_MATCH, 0);
  test("Moscow", "Red Square Moscw", TokenRange(2, 3), NAME_SCORE_FULL_MATCH, 1);
  test("San Francisco", "Fran", TokenRange(0, 1), NAME_SCORE_SUBSTRING, 0);
  test("San Francisco", "Fran ", TokenRange(0, 1), NAME_SCORE_ZERO, 0);
  test("San Francisco", "Sa", TokenRange(0, 1), NAME_SCORE_PREFIX, 0);
  test("San Francisco", "San ", TokenRange(0, 1), NAME_SCORE_PREFIX, 0);
  test("Лермонтовъ", "Лермон", TokenRange(0, 1), NAME_SCORE_PREFIX, 0);
  test("Лермонтовъ", "Лермонтов", TokenRange(0, 1), NAME_SCORE_FULL_MATCH, 1);
  test("Лермонтовъ", "Лермонтово", TokenRange(0, 1), NAME_SCORE_FULL_MATCH, 1);
  test("Лермонтовъ", "Лермнтовъ", TokenRange(0, 1), NAME_SCORE_FULL_MATCH, 1);
  test("фото на документы", "фото", TokenRange(0, 1), NAME_SCORE_PREFIX, 0);
  test("фотоателье", "фото", TokenRange(0, 1), NAME_SCORE_PREFIX, 0);
}

UNIT_TEST(PreferCountry)
{
  RankingInfo info;
  info.m_nameScore = NAME_SCORE_FULL_MATCH;
  info.m_errorsMade = ErrorsMade(0);
  info.m_numTokens = 1;
  info.m_matchedFraction = 1.0;
  info.m_allTokensUsed = true;
  info.m_exactMatch = true;

  auto cafe = info;
  cafe.m_distanceToPivot = 1e3;
  cafe.m_tokenRanges[Model::TYPE_SUBPOI] = TokenRange(0, 1);
  cafe.m_exactCountryOrCapital = false;
  cafe.m_type = Model::TYPE_SUBPOI;
  cafe.m_resultType = ResultType::Eat;

  auto country = info;
  country.m_distanceToPivot = 1e6;
  country.m_tokenRanges[Model::TYPE_COUNTRY] = TokenRange(0, 1);
  country.m_exactCountryOrCapital = true;
  country.m_type = Model::TYPE_COUNTRY;

  // Country should be preferred even if cafe is much closer to viewport center.
  TEST_LESS(cafe.GetLinearModelRank(), country.GetLinearModelRank(),());
}
}  // namespace
