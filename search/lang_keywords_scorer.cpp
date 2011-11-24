#include "lang_keywords_scorer.hpp"
#include "../indexer/search_string_utils.hpp"
#include "../indexer/search_delimiters.hpp"
#include "../base/stl_add.hpp"
#include "../std/algorithm.hpp"

namespace search
{

LangKeywordsScorer::LangKeywordsScorer(vector<vector<int8_t> > const & languagePriorities,
                                       strings::UniString const * keywords, size_t keywordCount,
                                       strings::UniString const * pPrefix)
  : m_languagePriorities(languagePriorities), m_keywordMatcher(keywords, keywordCount, pPrefix)
{
}

uint32_t LangKeywordsScorer::Score(int8_t lang, string const & name) const
{
  return Score(lang, NormalizeAndSimplifyString(name));
}

uint32_t LangKeywordsScorer::Score(int8_t lang, strings::UniString const & name) const
{
  buffer_vector<strings::UniString, MAX_TOKENS> tokens;
  SplitUniString(name, MakeBackInsertFunctor(tokens), Delimiters());
  ASSERT_LESS(tokens.size(), size_t(MAX_TOKENS), ());
  return Score(lang, tokens.data(), static_cast<int>(tokens.size()));
}

uint32_t LangKeywordsScorer::Score(int8_t lang,
                                   strings::UniString const * tokens, int tokenCount) const
{
  uint32_t keywordScore = m_keywordMatcher.Score(tokens, tokenCount);
  for (uint32_t i = 0; i < NUM_LANG_PRIORITY_TIERS; ++i)
    if (find(m_languagePriorities[i].begin(), m_languagePriorities[i].end(), lang) !=
        m_languagePriorities[i].end())
      return i * KeywordMatcher::MAX_SCORE + keywordScore;
  return NUM_LANG_PRIORITY_TIERS * KeywordMatcher::MAX_SCORE + keywordScore;
}

}  // namespace search
