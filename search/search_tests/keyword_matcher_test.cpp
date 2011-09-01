#include "../../testing/testing.hpp"
#include "../keyword_matcher.hpp"
#include "match_cost_mock.hpp"
#include "../approximate_string_match.hpp"
#include "../../indexer/search_string_utils.hpp"
#include "../../testing/testing_utils.hpp"
#include "../../base/string_utils.hpp"
#include "../../std/scoped_ptr.hpp"
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

struct KeywordMatcherAdaptor
{
  explicit KeywordMatcherAdaptor(char const * prefix,
                                 uint32_t maxKeywordMatchCost, uint32_t maxPrefixMatchCost,
                                 char const * s0, char const * s1 = NULL)
  {
    m_keywords.push_back(strings::MakeUniString(s0));
    if (s1)
      m_keywords.push_back(strings::MakeUniString(s1));
    for (size_t i = 0; i < m_keywords.size(); ++i)
      m_keywordPtrs.push_back(&m_keywords[i]);
    m_pMatcher.reset(new search::impl::KeywordMatcher(&m_keywordPtrs[0], m_keywordPtrs.size(),
                                                      strings::MakeUniString(prefix),
                                                      maxKeywordMatchCost, maxPrefixMatchCost,
                                                      &KeywordMatchForTest, &PrefixMatchForTest));
  }

  vector<strings::UniString> m_keywords;
  vector<strings::UniString const *> m_keywordPtrs;
  scoped_ptr<search::impl::KeywordMatcher> m_pMatcher;
};

}  // unnamed namespace

// TODO: KeywordMatcher tests.
/*
UNIT_TEST(KeywordMatcher_Smoke)
{
  KeywordMatcherAdaptor matcherAdaptor("l", 3, 3, "minsk", "belarus");
  search::impl::KeywordMatcher & matcher = *matcherAdaptor.m_pMatcher;
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 4, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + 2),
             Vec<uint32_t>(4, 4), ());
  TEST_EQUAL(matcher.GetMatchScore(), 4 + 4 + 4, ());

  matcher.ProcessName("belarrr");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 1, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + 2),
             Vec<uint32_t>(4, 2), ());
  TEST_EQUAL(matcher.GetMatchScore(), 1 + 4 + 2, ());

  matcher.ProcessName("belaruu minnn");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 1, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + 2),
             Vec<uint32_t>(2, 1), ());
  TEST_EQUAL(matcher.GetMatchScore(), 1 + 2 + 1, ());

  matcher.ProcessName("belaruu les minnn");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 0, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + 2),
             Vec<uint32_t>(2, 1), ());
  TEST_EQUAL(matcher.GetMatchScore(), 0 + 2 + 1, ());
}

UNIT_TEST(KeywordMatcher_NoPrefix)
{
  KeywordMatcherAdaptor matcherAdaptor("", 3, 3, "minsk", "belarus");
  search::impl::KeywordMatcher & matcher = *matcherAdaptor.m_pMatcher;
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 4, ());
  TEST_EQUAL(matcher.GetMatchScore(), 4 + 4 + 4, ());

  matcher.ProcessName("belaruu zzz minnn");
  TEST_EQUAL(matcher.GetPrefixMatchScore(), 0, ());
  TEST_EQUAL(vector<uint32_t>(matcher.GetKeywordMatchScores(),
                              matcher.GetKeywordMatchScores() + 1),
             Vec<uint32_t>(2, 1), ());
  TEST_EQUAL(matcher.GetMatchScore(), 0 + 2 + 1, ());
}

UNIT_TEST(KeywordMatcher_Suomi)
{
  KeywordMatcherAdaptor matcherAdaptor("", 4, 4, "minsk");
  search::impl::KeywordMatcher & matcher = *matcherAdaptor.m_pMatcher;
  matcher.ProcessName("Suomi");
  TEST_EQUAL(matcher.GetMatchScore(), 5, ());
}
*/
