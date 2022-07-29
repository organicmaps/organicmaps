#include "testing/testing.hpp"

#include "search/pre_ranker.hpp"
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

namespace ranking_tests
{
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

  params.Init(query, tokens, !query.empty() && !delims(strings::LastUniChar(query)));

  return GetNameScores(name, StringUtf8Multilang::kDefaultCode, TokenSlice(params, tokenRange));
}

void AssignRankingInfo(NameScores const & scores, RankingInfo & info, size_t totalLength)
{
  info.m_nameScore = scores.m_nameScore;
  info.m_errorsMade = scores.m_errorsMade;
  info.m_isAltOrOldName = scores.m_isAltOrOldName;
  info.m_matchedFraction = scores.m_matchedLength / static_cast<double>(totalLength);
}
} // namespace

UNIT_TEST(NameScore_Smoke)
{
  auto const test = [](string const & name, string const & query, TokenRange const & tokenRange,
                       NameScore nameScore, size_t errorsMade, size_t matchedLength)
  {
    NameScores const expected(nameScore, nameScore == NAME_SCORE_ZERO ? ErrorsMade() : ErrorsMade(errorsMade),
                              false /* isAltOrOldNAme */, matchedLength);
    TEST_EQUAL(GetScore(name, query, tokenRange), expected, (name, query, tokenRange));
  };

  base::ScopedLogLevelChanger const enableDebug(LDEBUG);
  //    name,      query,                        range,            expected score,   errors, match length
  test("New York", "Central Park, New York, US", TokenRange(2, 4), NAME_SCORE_FULL_MATCH, 0, 7);
  test("New York", "York", TokenRange(0, 1), NAME_SCORE_SUBSTRING, 0, 4);
  test("New York", "Chicago", TokenRange(0, 1), NAME_SCORE_ZERO, 0, 0);
  test("Moscow", "Red Square Mosc", TokenRange(2, 3), NAME_SCORE_PREFIX, 0, 4);
  test("Moscow", "Red Square Moscow", TokenRange(2, 3), NAME_SCORE_FULL_MATCH, 0, 6);
  test("Moscow", "Red Square Moscw", TokenRange(2, 3), NAME_SCORE_FULL_MATCH, 1, 5);
  test("San Francisco", "Fran", TokenRange(0, 1), NAME_SCORE_SUBSTRING, 0, 4);
  test("San Francisco", "Fran ", TokenRange(0, 1), NAME_SCORE_ZERO, 0, 0);
  test("San Francisco", "Sa", TokenRange(0, 1), NAME_SCORE_PREFIX, 0, 2);
  test("San Francisco", "San ", TokenRange(0, 1), NAME_SCORE_PREFIX, 0, 3);
  test("South Fredrick Street", "S Fredrick St", TokenRange(0, 3), NAME_SCORE_FULL_MATCH, 0, 11);
  test("South Fredrick Street", "S Fredrick", TokenRange(0, 2), NAME_SCORE_PREFIX, 0, 9);
  test("South Fredrick Street", "Fredrick St", TokenRange(0, 2), NAME_SCORE_SUBSTRING, 0, 10);
  test("North Scott Boulevard", "N Scott Blvd", TokenRange(0, 3), NAME_SCORE_FULL_MATCH, 0, 10);
  test("North Scott Boulevard", "N Scott", TokenRange(0, 2), NAME_SCORE_PREFIX, 0, 6);
  test("Лермонтовъ", "Лермон", TokenRange(0, 1), NAME_SCORE_PREFIX, 0, 6);
  test("Лермонтовъ", "Лермонтов", TokenRange(0, 1), NAME_SCORE_PREFIX, 0, 9);
  test("Лермонтовъ", "Лермонтово", TokenRange(0, 1), NAME_SCORE_FULL_MATCH, 1, 10);
  test("Лермонтовъ", "Лермнтовъ", TokenRange(0, 1), NAME_SCORE_FULL_MATCH, 1, 9);
  test("фото на документы", "фото", TokenRange(0, 1), NAME_SCORE_PREFIX, 0, 4);
  test("фотоателье", "фото", TokenRange(0, 1), NAME_SCORE_PREFIX, 0, 4);
}

UNIT_TEST(NameScore_SubstringVsErrors)
{
  {
    string const query = "Simon";
    TokenRange const range(0, 1);

    RankingInfo info;
    info.m_tokenRanges[Model::TYPE_SUBPOI] = range;
    info.m_numTokens = 1;
    info.m_allTokensUsed = true;
    info.m_exactMatch = false;
    info.m_exactCountryOrCapital = false;

    RankingInfo poi1 = info;
    AssignRankingInfo(GetScore("Symon Budny and Vasil Tsiapinski", query, range), poi1, query.size());
    TEST_EQUAL(poi1.m_nameScore, NAME_SCORE_PREFIX, ());
    TEST_EQUAL(poi1.m_errorsMade, ErrorsMade(1), ());

    RankingInfo poi2 = info;
    AssignRankingInfo(GetScore("Church of Saints Simon and Helen", query, range), poi2, query.size());
    TEST_EQUAL(poi2.m_nameScore, NAME_SCORE_SUBSTRING, ());
    TEST_EQUAL(poi2.m_errorsMade, ErrorsMade(0), ());

    TEST_LESS(poi1.GetLinearModelRank(), poi2.GetLinearModelRank(), ());
  }
}

UNIT_TEST(RankingInfo_PreferCountry)
{
  RankingInfo info;
  info.m_nameScore = NAME_SCORE_FULL_MATCH;
  info.m_errorsMade = ErrorsMade(0);
  info.m_numTokens = 1;
  info.m_matchedFraction = 1.0;
  info.m_allTokensUsed = true;
  info.m_exactMatch = false;

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
  TEST_LESS(cafe.GetLinearModelRank(), country.GetLinearModelRank(), ());
}

UNIT_TEST(RankingInfo_PrefixVsFull)
{
  RankingInfo info;
  info.m_numTokens = 3;
  info.m_matchedFraction = 1.0;
  info.m_allTokensUsed = true;
  info.m_exactMatch = false;
  info.m_exactCountryOrCapital = false;
  info.m_distanceToPivot = 1000;
  info.m_tokenRanges[Model::TYPE_SUBPOI] = TokenRange(0, 2);

  {
    // Ensure that NAME_SCORE_PREFIX with 0 errors is better than NAME_SCORE_FULL_MATCH with 1 error.

    auto full = info;
    full.m_nameScore = NAME_SCORE_FULL_MATCH;
    full.m_errorsMade = ErrorsMade(1);

    auto prefix = info;
    prefix.m_nameScore = NAME_SCORE_PREFIX;
    prefix.m_errorsMade = ErrorsMade(0);

    TEST_LESS(full.GetLinearModelRank(), prefix.GetLinearModelRank(), ());
  }
}

namespace
{
class MwmIdWrapper
{
  FeatureID m_id;
public:
  MwmIdWrapper(MwmSet::MwmId id) : m_id(move(id), 0) {}
  FeatureID const & GetId() const { return m_id; }
};

size_t UniqueMwmIdCount(std::vector<MwmIdWrapper> & test)
{
  set<MwmSet::MwmId> mwmSet;
  size_t count = 0;
  MwmSet::MwmId curr;
  PreRanker::ForEachMwmOrder(test, [&](MwmIdWrapper & w)
  {
    auto const & id = w.GetId().m_mwmId;
    if (curr != id)
    {
      curr = id;
      TEST(mwmSet.insert(curr).second, ());
    }
    ++count;
  });

  TEST_EQUAL(count, test.size(), ());
  return mwmSet.size();
}
} // namespace

UNIT_TEST(PreRanker_ForEachMwmOrder)
{
  MwmSet::MwmId id1(make_shared<MwmInfo>());
  MwmSet::MwmId id2(make_shared<MwmInfo>());
  MwmSet::MwmId id3(make_shared<MwmInfo>());

  {
    std::vector<MwmIdWrapper> test{id1, id1};
    TEST_EQUAL(1, UniqueMwmIdCount(test), ());
  }

  {
    std::vector<MwmIdWrapper> test{id1, id2, id1, id3, id2};
    TEST_EQUAL(3, UniqueMwmIdCount(test), ());
  }
}
} // namespace ranking_tests
