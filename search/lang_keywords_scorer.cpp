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
  uint32_t const keywordScore = m_keywordMatcher.Score(tokens, tokenCount);
  if (keywordScore == KeywordMatcher::MAX_SCORE)
    return MAX_SCORE; // TODO: Differentiate between langs with MAX_SCORE score.

  for (uint32_t i = 0; i < NUM_LANG_PRIORITY_TIERS; ++i)
    for (uint32_t j = 0; j < m_languagePriorities[i].size(); ++j)
      if (m_languagePriorities[i][j] == lang)
        return i * KeywordMatcher::MAX_SCORE * (MAX_LANGS_IN_TIER + 1)
            + keywordScore * (MAX_LANGS_IN_TIER + 1)
            + min(j, static_cast<uint32_t>(MAX_LANGS_IN_TIER));

  return NUM_LANG_PRIORITY_TIERS * KeywordMatcher::MAX_SCORE * (MAX_LANGS_IN_TIER + 1)
      + keywordScore * (MAX_LANGS_IN_TIER + 1);
}

}  // namespace search
