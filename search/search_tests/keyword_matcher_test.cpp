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

    m_matcher.SetKeywords(m_keywords.data(), m_keywords.size(), &m_prefix);
  }

  search::KeywordMatcher m_matcher;
private:
  buffer_vector<strings::UniString, 10> m_keywords;
  strings::UniString m_prefix;
};

}  // unnamed namespace

UNIT_TEST(KeywordMatcher_New)
{
  Matcher matcher("new ");
  TEST_EQUAL(matcher.m_matcher.Score("new"), 0, ());
  TEST_EQUAL(matcher.m_matcher.Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_matcher.Score("new york"), 0, ());
}

UNIT_TEST(KeywordMatcher_York)
{
  Matcher matcher("york ");
  TEST_EQUAL(matcher.m_matcher.Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_matcher.Score("york"), 0, ());
  TEST_EQUAL(matcher.m_matcher.Score("new york"), 1, ());
}

UNIT_TEST(KeywordMatcher_NewYork)
{
  Matcher matcher1("new york ");
  Matcher matcher2("new york");
  TEST_EQUAL(matcher1.m_matcher.Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher2.m_matcher.Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher1.m_matcher.Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher2.m_matcher.Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher1.m_matcher.Score("new york"), 0, ());
  TEST_EQUAL(matcher2.m_matcher.Score("new york"), 0, ());
}

UNIT_TEST(KeywordMatcher_YorkNew)
{
  Matcher matcher("new york ");
  TEST_EQUAL(matcher.m_matcher.Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_matcher.Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_matcher.Score("new york"), 0, ());
}

UNIT_TEST(KeywordMatcher_NewYo)
{
  Matcher matcher("new yo");
  TEST_EQUAL(matcher.m_matcher.Score("new"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_matcher.Score("york"), MAX_SCORE, ());
  TEST_EQUAL(matcher.m_matcher.Score("new york"), 0, ());
}
