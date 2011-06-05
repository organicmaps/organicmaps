#include "../../testing/testing.hpp"
#include "../keyword_matcher.hpp"
#include "match_cost_mock.hpp"
#include "../string_match.hpp"
#include "../../testing/testing_utils.hpp"
#include "../../base/string_utils.hpp"
#include "../../std/vector.hpp"

namespace
{

uint32_t KeywordMatchForTest(strings::UniChar const * sA, uint32_t sizeA,
                             strings::UniChar const * sB, uint32_t sizeB,
                             uint32_t maxCost)
{
  return StringMatchCost(sA, sizeA, sB, sizeB, search::MatchCostMock<strings::UniChar>(),
                         maxCost, false);
}

uint32_t PrefixMatchForTest(strings::UniChar const * sA, uint32_t sizeA,
                            strings::UniChar const * sB, uint32_t sizeB,
                            uint32_t maxCost)
{
  return StringMatchCost(sA, sizeA, sB, sizeB, search::MatchCostMock<strings::UniChar>(),
                         maxCost, true);
}

}  // unnamed namespace

UNIT_TEST(KeywordMatcher_Smoke)
{
  vector<strings::UniString> keywords;
  keywords.push_back(strings::MakeUniString("minsk"));
  keywords.push_back(strings::MakeUniString("belarus"));
  search::impl::KeywordMatcher matcher(&keywords[0], keywords.size(),
                                       strings::MakeUniString("l"),
                                       3, 3,
                                       &KeywordMatchForTest, &PrefixMatchForTest);
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 4, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + keywords.size()),
             Vec<uint32_t>(4, 4), ());
  TEST_EQUAL(matcher.GetMatchScore(), 4 + 4 + 4, ());

  matcher.ProcessName("belarrr");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 1, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + keywords.size()),
             Vec<uint32_t>(4, 2), ());
  TEST_EQUAL(matcher.GetMatchScore(), 1 + 4 + 2, ());

  matcher.ProcessName("belaruu minnn");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 1, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + keywords.size()),
             Vec<uint32_t>(2, 1), ());
  TEST_EQUAL(matcher.GetMatchScore(), 1 + 2 + 1, ());

  matcher.ProcessName("belaruu les minnn");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 0, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + keywords.size()),
             Vec<uint32_t>(2, 1), ());
  TEST_EQUAL(matcher.GetMatchScore(), 0 + 2 + 1, ());
}

UNIT_TEST(KeywordMatcher_NoPrefix)
{
  vector<strings::UniString> keywords;
  keywords.push_back(strings::MakeUniString("minsk"));
  keywords.push_back(strings::MakeUniString("belarus"));
  search::impl::KeywordMatcher matcher(&keywords[0], keywords.size(),
                                       strings::MakeUniString(""),
                                       3, 3,
                                       &KeywordMatchForTest, &PrefixMatchForTest);
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 4, ());
  TEST_EQUAL(matcher.GetMatchScore(), 4 + 4 + 4, ());

  matcher.ProcessName("belaruu zzz minnn");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 0, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + keywords.size()),
             Vec<uint32_t>(2, 1), ());
  TEST_EQUAL(matcher.GetMatchScore(), 0 + 2 + 1, ());
}

UNIT_TEST(KeywordMatcher_Suomi)
{
  vector<strings::UniString> keywords;
  keywords.push_back(strings::MakeUniString("minsk"));
  search::impl::KeywordMatcher matcher(&keywords[0], keywords.size(),
                                       strings::MakeUniString(""),
                                       4, 4,
                                       &KeywordMatchForTest, &PrefixMatchForTest);
  matcher.ProcessName("Suomi");
  TEST_EQUAL(matcher.GetMatchScore(), 5, ());
}
