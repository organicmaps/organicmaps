#include "testing/testing.hpp"
#include "search/keyword_lang_matcher.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/stl_add.hpp"

#include "std/vector.hpp"

namespace
{

using search::KeywordLangMatcher;
typedef search::KeywordLangMatcher::ScoreT ScoreT;

enum
{
  LANG_UNKNOWN = 1,
  LANG_SOME = 2,
  LANG_SOME_OTHER = 3,
  LANG_HIGH_PRIORITY = 10
};

KeywordLangMatcher CreateMatcher(string const & query)
{
  KeywordLangMatcher matcher;

  vector<vector<int8_t> > langPriorities(4, vector<int8_t>());
  langPriorities[0].push_back(LANG_HIGH_PRIORITY);
  // langPriorities[1] is intentionally left empty.
  langPriorities[2].push_back(LANG_SOME);
  langPriorities[2].push_back(LANG_SOME_OTHER);
  // langPriorities[3] is intentionally left empty.
  matcher.SetLanguages(langPriorities);

  vector<strings::UniString> keywords;
  strings::UniString prefix;
  if (search::TokenizeStringAndCheckIfLastTokenIsPrefix(query, keywords, search::Delimiters()))
  {
    prefix = keywords.back();
    keywords.pop_back();
  }
  matcher.SetKeywords(&keywords[0], keywords.size(), prefix);

  return matcher;
}

}  // unnamed namespace


UNIT_TEST(KeywordMatcher_TokensMatchHasPriority)
{
}

UNIT_TEST(KeywordMatcher_LanguageMatchIsUsedWhenTokenMatchIsTheSame)
{
  char const * query = "test";
  char const * name = "test";
  KeywordLangMatcher matcher = CreateMatcher(query);

  TEST(matcher.Score(LANG_UNKNOWN, name) < matcher.Score(LANG_SOME, name), ());
  TEST(matcher.Score(LANG_UNKNOWN, name) < matcher.Score(LANG_SOME_OTHER, name), ());
  TEST(matcher.Score(LANG_UNKNOWN, name) < matcher.Score(LANG_HIGH_PRIORITY, name), ());

  TEST(!(matcher.Score(LANG_SOME, name) < matcher.Score(LANG_SOME_OTHER, name)), ());
  TEST(!(matcher.Score(LANG_SOME_OTHER, name) < matcher.Score(LANG_SOME, name)), ());

  TEST(matcher.Score(LANG_SOME, name) < matcher.Score(LANG_HIGH_PRIORITY, name), ());
  TEST(matcher.Score(LANG_SOME_OTHER, name) < matcher.Score(LANG_HIGH_PRIORITY, name), ());
}
