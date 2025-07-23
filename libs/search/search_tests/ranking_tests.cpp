#include "testing/testing.hpp"

#include "search/pre_ranker.hpp"
#include "search/query_params.hpp"
#include "search/ranking_info.hpp"
#include "search/ranking_utils.hpp"
#include "search/token_range.hpp"
#include "search/token_slice.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ranking_tests
{
using namespace search;
using namespace std;

namespace
{
NameScores GetScore(string const & name, string const & query)
{
  Delimiters delims;
  QueryParams params;

  auto const tokens = NormalizeAndTokenizeString(query);

  params.Init(query, tokens, !query.empty() && !delims(strings::LastUniChar(query)));

  return GetNameScores(name, StringUtf8Multilang::kDefaultCode, TokenSlice(params, {0, tokens.size()}));
}

void AssignRankingInfo(NameScores const & scores, RankingInfo & info, size_t totalLength)
{
  info.m_nameScore = scores.m_nameScore;
  info.m_errorsMade = scores.m_errorsMade;
  info.m_isAltOrOldName = scores.m_isAltOrOldName;
  info.m_matchedFraction = scores.m_matchedLength / static_cast<float>(totalLength);
}
}  // namespace

UNIT_TEST(NameScore_Smoke)
{
  auto const test =
      [](string const & name, string const & query, NameScore nameScore, size_t errorsMade, size_t matchedLength)
  {
    NameScores const expected(nameScore, nameScore == NameScore::ZERO ? ErrorsMade() : ErrorsMade(errorsMade),
                              false /* isAltOrOldNAme */, matchedLength);
    TEST_EQUAL(GetScore(name, query), expected, (name, query));
  };

  base::ScopedLogLevelChanger const enableDebug(LDEBUG);
  //    name,      query,      expected score, errors, match length
  test("New York", "New York", NameScore::FULL_MATCH, 0, 7);
  test("New York", "York", NameScore::SUBSTRING, 0, 4);
  test("New York", "Chicago", NameScore::ZERO, 0, 0);
  test("Moscow", "Mosc", NameScore::PREFIX, 0, 4);
  test("Moscow", "Moscow", NameScore::FULL_MATCH, 0, 6);
  test("Moscow", "Moscw", NameScore::FULL_MATCH, 1, 5);
  test("San Francisco", "Fran", NameScore::SUBSTRING, 0, 4);
  test("San Francisco", "Fran ", NameScore::ZERO, 0, 0);
  test("San Francisco", "Sa", NameScore::PREFIX, 0, 2);
  test("San Francisco", "San ", NameScore::FULL_PREFIX, 0, 3);
  test("San Francisco", "san fr", NameScore::PREFIX, 0, 5);
  test("San Francisco", "san fracis", NameScore::PREFIX, 1, 9);
  test("South Fredrick Street", "S Fredrick St", NameScore::FULL_MATCH, 0, 11);
  test("South Fredrick Street", "S Fredrick", NameScore::FULL_PREFIX, 0, 9);
  test("South Fredrick Street", "Fredrick St", NameScore::SUBSTRING, 0, 10);
  test("North Scott Boulevard", "N Scott Blvd", NameScore::FULL_MATCH, 0, 10);
  test("North Scott Boulevard", "N Scott", NameScore::FULL_PREFIX, 0, 6);
  test("North Scott Boulevard", "N Sco", NameScore::PREFIX, 0, 4);
  test("Лермонтовъ", "Лермон", NameScore::PREFIX, 0, 6);
  test("Лермонтовъ", "Лермонтов", NameScore::PREFIX, 0, 9);
  test("Лермонтовъ", "Лермонтово", NameScore::FULL_MATCH, 1, 10);
  test("Лермонтовъ", "Лермнтовъ", NameScore::FULL_MATCH, 1, 9);
  test("фото на документы", "фото", NameScore::FULL_PREFIX, 0, 4);
  test("фотоателье", "фото", NameScore::PREFIX, 0, 4);

  test("Pennsylvania Ave NW, Washington, DC", "1600 Pennsylvania Ave", NameScore::SUBSTRING, 0, 15);
  test("Pennsylvania Ave NW, Washington, DC", "Pennsylvania Ave, Chicago", NameScore::FIRST_MATCH, 0, 15);

  test("Barnes & Noble", "barne & noble", NameScore::FULL_MATCH, 1, 10);
  test("Barnes Avenue", "barne ", NameScore::FULL_PREFIX, 1, 5);
  test("Barnes Avenue", "barne & noble", NameScore::FIRST_MATCH, 1, 5);

  test("Barnes Avenue", "barne's & noble", NameScore::FIRST_MATCH, 0, 6);
  test("Barnes & Noble", "barne's & noble", NameScore::FULL_MATCH, 0, 11);
  test("Barne's & Noble", "barnes & noble", NameScore::FULL_MATCH, 0, 11);

  test("Зона №51", "зона 51", NameScore::FULL_MATCH, 0, 6);
  test("Зона №51", "зона №", NameScore::FULL_PREFIX, 0, 4);

  test("Göztepe 60. Yıl Parkı", "goztepe parki", NameScore::FIRST_MATCH, 0, 12);
  test("Göztepe 60. Yıl Parkı", "goztepe 60 parki", NameScore::FIRST_MATCH, 0, 14);
  test("Göztepe 60. Yıl Parkı", "60 parki", NameScore::SUBSTRING, 0, 7);
  test("Göztepe 60. Yıl Parkı", "yil parki", NameScore::SUBSTRING, 0, 8);

  test("Mariano Acosta", "arcos", NameScore::SUBSTRING, 1, 5);  /// @todo PREFIX?
  /// @todo Matched, rank calculation is bad.
  // test("Marcos Paz", "arcos", NameScore::FULL_PREFIX, 1, 5);
}

namespace
{
ErrorsMade GetErrorsMade(QueryParams::Token const & token, strings::UniString const & text)
{
  return search::impl::GetErrorsMade(token, text, search::BuildLevenshteinDFA(text));
}
ErrorsMade GetPrefixErrorsMade(QueryParams::Token const & token, strings::UniString const & text)
{
  return search::impl::GetPrefixErrorsMade(token, text, search::BuildLevenshteinDFA(text));
}
}  // namespace

UNIT_TEST(ErrorsMade_Smoke)
{
  {
    QueryParams::Token const searchToken = strings::MakeUniString("hairdressers");

    auto nameToken = strings::MakeUniString("h");
    TEST(!GetErrorsMade(searchToken, nameToken).IsValid(), ());
    TEST(!GetPrefixErrorsMade(searchToken, nameToken).IsValid(), ());

    nameToken = strings::MakeUniString("hair");
    TEST(!GetErrorsMade(searchToken, nameToken).IsValid(), ());
    TEST(!GetPrefixErrorsMade(searchToken, nameToken).IsValid(), ());
  }

  {
    auto nameToken = strings::MakeUniString("hair");

    QueryParams::Token searchToken = strings::MakeUniString("hair");
    TEST_EQUAL(GetErrorsMade(searchToken, nameToken).m_errorsMade, 0, ());
    TEST_EQUAL(GetPrefixErrorsMade(searchToken, nameToken).m_errorsMade, 0, ());

    searchToken = strings::MakeUniString("gair");
    TEST_EQUAL(GetErrorsMade(searchToken, nameToken).m_errorsMade, 1, ());
    TEST_EQUAL(GetPrefixErrorsMade(searchToken, nameToken).m_errorsMade, 1, ());

    searchToken = strings::MakeUniString("gai");
    TEST(!GetErrorsMade(searchToken, nameToken).IsValid(), ());
    TEST_EQUAL(GetPrefixErrorsMade(searchToken, nameToken).m_errorsMade, 1, ());

    searchToken = strings::MakeUniString("hairrr");
    TEST(!GetErrorsMade(searchToken, nameToken).IsValid(), ());
    TEST(!GetPrefixErrorsMade(searchToken, nameToken).IsValid(), ());
  }

  {
    auto nameToken = strings::MakeUniString("hairdresser");

    QueryParams::Token searchToken = strings::MakeUniString("hair");
    TEST(!GetErrorsMade(searchToken, nameToken).IsValid(), ());
    TEST_EQUAL(GetPrefixErrorsMade(searchToken, nameToken).m_errorsMade, 0, ());

    searchToken = strings::MakeUniString("gair");
    TEST_EQUAL(GetPrefixErrorsMade(searchToken, nameToken).m_errorsMade, 1, ());

    searchToken = strings::MakeUniString("gairdrese");
    TEST(!GetErrorsMade(searchToken, nameToken).IsValid(), ());
    TEST_EQUAL(GetPrefixErrorsMade(searchToken, nameToken).m_errorsMade, 2, ());
  }
}

UNIT_TEST(NameScore_Prefix)
{
  TEST_EQUAL(GetScore("H Nicks", "hairdressers").m_nameScore, NameScore::ZERO, ());
  TEST_EQUAL(GetScore("Hair E14", "hairdressers").m_nameScore, NameScore::ZERO, ());
}

UNIT_TEST(NameScore_SubstringVsErrors)
{
  string const query = "Simon";

  RankingInfo info;
  info.m_type = Model::TYPE_SUBPOI;
  info.m_tokenRanges[Model::TYPE_SUBPOI] = {0, 1};
  info.m_numTokens = 1;
  info.m_allTokensUsed = true;
  info.m_exactMatch = false;

  {
    RankingInfo poi1 = info;
    AssignRankingInfo(GetScore("Symon Budny and Vasil Tsiapinski", query), poi1, query.size());
    TEST_EQUAL(poi1.m_nameScore, NameScore::FULL_PREFIX, ());
    TEST_EQUAL(poi1.m_errorsMade, ErrorsMade(1), ());

    RankingInfo poi2 = info;
    AssignRankingInfo(GetScore("Church of Saints Simon and Helen", query), poi2, query.size());
    TEST_EQUAL(poi2.m_nameScore, NameScore::SUBSTRING, ());
    TEST_EQUAL(poi2.m_errorsMade, ErrorsMade(0), ());

    TEST_LESS(poi1.GetLinearModelRank(), poi2.GetLinearModelRank(), (poi1, poi2));
  }
}

UNIT_TEST(RankingInfo_PreferCountry)
{
  RankingInfo info;
  info.m_nameScore = NameScore::FULL_MATCH;
  info.m_errorsMade = ErrorsMade(0);
  info.m_numTokens = 1;
  info.m_matchedFraction = 1;
  info.m_allTokensUsed = true;
  info.m_exactMatch = false;

  auto cafe = info;
  cafe.m_distanceToPivot = 1e3;
  cafe.m_tokenRanges[Model::TYPE_SUBPOI] = TokenRange(0, 1);
  cafe.m_type = Model::TYPE_SUBPOI;
  cafe.m_classifType.poi = PoiType::Eat;

  auto country = info;
  country.m_distanceToPivot = 1e6;
  country.m_tokenRanges[Model::TYPE_COUNTRY] = TokenRange(0, 1);
  country.m_type = Model::TYPE_COUNTRY;
  country.m_rank = 100;  // This is rather small rank for a country.

  // Country should be preferred even if cafe is much closer to viewport center.
  TEST_LESS(cafe.GetLinearModelRank(), country.GetLinearModelRank(), (cafe, country));
}

UNIT_TEST(RankingInfo_PrefixVsFull)
{
  RankingInfo info;
  info.m_numTokens = 3;
  info.m_matchedFraction = 1;
  info.m_allTokensUsed = true;
  info.m_exactMatch = false;
  info.m_distanceToPivot = 1000;
  info.m_type = Model::TYPE_SUBPOI;
  info.m_tokenRanges[Model::TYPE_SUBPOI] = TokenRange(0, 2);

  {
    // Ensure that NameScore::PREFIX with 0 errors is better than NameScore::FULL_MATCH with 1 error.

    auto full = info;
    full.m_nameScore = NameScore::FULL_MATCH;
    full.m_errorsMade = ErrorsMade(1);

    auto prefix = info;
    prefix.m_nameScore = NameScore::PREFIX;
    prefix.m_errorsMade = ErrorsMade(0);

    TEST_LESS(full.GetLinearModelRank(), prefix.GetLinearModelRank(), (full, prefix));
  }
}

namespace
{
class MwmIdWrapper
{
  FeatureID m_id;

public:
  MwmIdWrapper(MwmSet::MwmId id) : m_id(std::move(id), 0) {}
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
}  // namespace

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
}  // namespace ranking_tests
