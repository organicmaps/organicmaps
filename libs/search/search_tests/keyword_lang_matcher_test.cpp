#include "testing/testing.hpp"

#include "search/keyword_lang_matcher.hpp"
#include "search/string_utils.hpp"

#include <vector>

namespace keyword_lang_matcher_test
{
using namespace std;

using search::KeywordLangMatcher;
using Score = search::KeywordLangMatcher::Score;

enum
{
  LANG_UNKNOWN = 1,
  LANG_SOME = 2,
  LANG_SOME_OTHER = 3,
  LANG_HIGH_PRIORITY = 10
};

KeywordLangMatcher CreateMatcher(string const & query)
{
  size_t const kNumTestTiers = 4;
  KeywordLangMatcher matcher(kNumTestTiers);

  {
    vector<vector<int8_t>> langPriorities(kNumTestTiers);
    langPriorities[0].push_back(LANG_HIGH_PRIORITY);
    // langPriorities[1] is intentionally left empty.
    langPriorities[2].push_back(LANG_SOME);
    langPriorities[2].push_back(LANG_SOME_OTHER);
    // langPriorities[3] is intentionally left empty.

    for (size_t i = 0; i < langPriorities.size(); ++i)
      matcher.SetLanguages(i /* tier */, std::move(langPriorities[i]));
  }

  matcher.SetKeywords(search::MakeQueryString(query));
  return matcher;
}

UNIT_TEST(KeywordMatcher_LanguageMatchIsUsedWhenTokenMatchIsTheSame)
{
  char const * query = "test";
  char const * name = "test";
  KeywordLangMatcher matcher = CreateMatcher(query);

  TEST(matcher.CalcScore(LANG_UNKNOWN, name) < matcher.CalcScore(LANG_SOME, name), ());
  TEST(matcher.CalcScore(LANG_UNKNOWN, name) < matcher.CalcScore(LANG_SOME_OTHER, name), ());
  TEST(matcher.CalcScore(LANG_UNKNOWN, name) < matcher.CalcScore(LANG_HIGH_PRIORITY, name), ());

  TEST(!(matcher.CalcScore(LANG_SOME, name) < matcher.CalcScore(LANG_SOME_OTHER, name)), ());
  TEST(!(matcher.CalcScore(LANG_SOME_OTHER, name) < matcher.CalcScore(LANG_SOME, name)), ());

  TEST(matcher.CalcScore(LANG_SOME, name) < matcher.CalcScore(LANG_HIGH_PRIORITY, name), ());
  TEST(matcher.CalcScore(LANG_SOME_OTHER, name) < matcher.CalcScore(LANG_HIGH_PRIORITY, name), ());
}

}  // namespace keyword_lang_matcher_test
