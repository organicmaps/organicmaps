#include "../../testing/testing.hpp"
#include "../keyword_matcher.hpp"
#include "../../indexer/search_string_utils.hpp"
#include "../../indexer/search_delimiters.hpp"
#include "../../base/buffer_vector.hpp"
#include "../../base/stl_add.hpp"
#include "../../std/scoped_ptr.hpp"

namespace
{
static const uint32_t MAX_SCORE = search::KeywordMatcher::MAX_SCORE;

class Matcher
{
public:
  Matcher(char const * query)
  {
    strings::UniString const uniQuery = search::NormalizeAndSimplifyString(query);
    SplitUniString(uniQuery, MakeBackInsertFunctor(m_keywords), search::Delimiters());
    if (!uniQuery.empty() && uniQuery.back() != ' ')
    {
      m_prefix = m_keywords.back();
      m_keywords.pop_back();
    }
    m_ptrs.resize(m_keywords.size());
    for (size_t i = 0; i < m_keywords.size(); ++i)
      m_ptrs[i] = &m_keywords[i];
    m_pMatcher.reset(new search::KeywordMatcher(m_ptrs.data(), int(m_ptrs.size()), &m_prefix));
  }

  scoped_ptr<search::KeywordMatcher> m_pMatcher;
private:
  buffer_vector<strings::UniString, 10> m_keywords;
  buffer_vector<strings::UniString const *, 10> m_ptrs;
  strings::UniString m_prefix;
};

}  // unnamed namespace
UNIT_TEST(KeywordMatcher_New)
{
  Matcher matcher("new ");
  TEST_EQUAL(matcher.m_pMatcher->Score("new"), 0, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("new york"), 0, ());
}

UNIT_TEST(KeywordMatcher_York)
{
  Matcher matcher("york ");
  TEST_EQUAL(matcher.m_pMatcher->Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("york"), 0, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("new york"), 1, ());
}

UNIT_TEST(KeywordMatcher_NewYork)
{
  Matcher matcher1("new york ");
  Matcher matcher2("new york");
  TEST_EQUAL(matcher1.m_pMatcher->Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher2.m_pMatcher->Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher1.m_pMatcher->Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher2.m_pMatcher->Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher1.m_pMatcher->Score("new york"), 0, ());
  TEST_EQUAL(matcher2.m_pMatcher->Score("new york"), 0, ());
}

UNIT_TEST(KeywordMatcher_YorkNew)
{
  Matcher matcher("new york ");
  TEST_EQUAL(matcher.m_pMatcher->Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("new york"), 0, ());
}

UNIT_TEST(KeywordMatcher_NewYo)
{
  Matcher matcher("new yo");
  TEST_EQUAL(matcher.m_pMatcher->Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_pMatcher->Score("new york"), 0, ());
}









