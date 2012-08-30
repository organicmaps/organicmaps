#include "lang_keywords_scorer.hpp"

#include "../indexer/search_string_utils.hpp"
#include "../indexer/search_delimiters.hpp"

#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"


namespace search
{

void LangKeywordsScorer::SetLanguages(vector<vector<int8_t> > const & languagePriorities)
{
  m_languagePriorities = languagePriorities;

#ifdef DEBUG
  ASSERT_EQUAL ( static_cast<size_t>(NUM_LANG_PRIORITY_TIERS), m_languagePriorities.size(), () );
  for (int i = 0; i < NUM_LANG_PRIORITY_TIERS; ++i)
    ASSERT_LESS_OR_EQUAL ( m_languagePriorities[i].size(), static_cast<size_t>(MAX_LANGS_IN_TIER), () );
#endif
}

bool LangKeywordsScorer::AssertIndex(pair<int, int> const & ind) const
{
  ASSERT_LESS ( static_cast<size_t>(ind.first), m_languagePriorities.size(), () );
  ASSERT_LESS ( static_cast<size_t>(ind.second), m_languagePriorities[ind.first].size(), () );
  return true;
}

void LangKeywordsScorer::SetLanguage(pair<int, int> const & ind, int8_t lang)
{
  ASSERT ( AssertIndex(ind), () );
  m_languagePriorities[ind.first][ind.second] = lang;
}

int8_t LangKeywordsScorer::GetLanguage(pair<int, int> const & ind) const
{
  ASSERT ( AssertIndex(ind), () );
  return m_languagePriorities[ind.first][ind.second];
}

uint32_t LangKeywordsScorer::Score(int8_t lang, string const & name) const
{
  return Score(lang, NormalizeAndSimplifyString(name));
}

uint32_t LangKeywordsScorer::Score(int8_t lang, StringT const & name) const
{
  buffer_vector<StringT, MAX_TOKENS> tokens;
  SplitUniString(name, MakeBackInsertFunctor(tokens), Delimiters());

  /// @todo Some Arabian names have a lot of tokens.
  /// Trim this stuff while generation.
  //ASSERT_LESS ( tokens.size(), static_cast<size_t>(MAX_TOKENS), () );

  return Score(lang, tokens.data(), min(size_t(MAX_TOKENS-1), tokens.size()));
}

uint32_t LangKeywordsScorer::Score(int8_t lang, StringT const * tokens, size_t count) const
{
  uint32_t const keywordScore = m_keywordMatcher.Score(tokens, count);

  // get score by language priority
  uint32_t const factor = KeywordMatcher::MAX_SCORE * MAX_LANGS_IN_TIER;
  uint32_t const value = keywordScore * MAX_LANGS_IN_TIER;

  for (uint32_t i = 0; i < NUM_LANG_PRIORITY_TIERS; ++i)
    for (uint32_t j = 0; j < m_languagePriorities[i].size(); ++j)
      if (m_languagePriorities[i][j] == lang)
        return (i * factor + value + j);

  return (NUM_LANG_PRIORITY_TIERS * factor);
}

}  // namespace search
