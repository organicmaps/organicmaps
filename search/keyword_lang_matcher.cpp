#include "keyword_lang_matcher.hpp"

#include "../indexer/search_string_utils.hpp"
#include "../indexer/search_delimiters.hpp"

#include "../base/stl_add.hpp"

#include "../std/algorithm.hpp"


namespace search
{

KeywordLangMatcher::ScoreT::ScoreT(KeywordMatcher::ScoreT const & score, int langScore)
  : m_parentScore(score), m_langScore(langScore)
{
}

bool KeywordLangMatcher::ScoreT::operator <(KeywordLangMatcher::ScoreT const & score) const
{
  if (m_parentScore < score.m_parentScore)
    return true;
  if (score.m_parentScore < m_parentScore)
    return false;

  if (m_langScore != score.m_langScore)
    return m_langScore < score.m_langScore;

  return false;
}

void KeywordLangMatcher::SetLanguages(vector<vector<int8_t> > const & languagePriorities)
{
  m_languagePriorities = languagePriorities;

#ifdef DEBUG
  ASSERT_EQUAL ( static_cast<size_t>(NUM_LANG_PRIORITY_TIERS), m_languagePriorities.size(), () );
  for (int i = 0; i < NUM_LANG_PRIORITY_TIERS; ++i)
    ASSERT_LESS_OR_EQUAL ( m_languagePriorities[i].size(), static_cast<size_t>(MAX_LANGS_IN_TIER), () );
#endif
}

bool KeywordLangMatcher::AssertIndex(pair<int, int> const & ind) const
{
  ASSERT_LESS ( static_cast<size_t>(ind.first), m_languagePriorities.size(), () );
  ASSERT_LESS ( static_cast<size_t>(ind.second), m_languagePriorities[ind.first].size(), () );
  return true;
}

void KeywordLangMatcher::SetLanguage(pair<int, int> const & ind, int8_t lang)
{
  ASSERT ( AssertIndex(ind), () );
  m_languagePriorities[ind.first][ind.second] = lang;
}

int8_t KeywordLangMatcher::GetLanguage(pair<int, int> const & ind) const
{
  ASSERT ( AssertIndex(ind), () );
  return m_languagePriorities[ind.first][ind.second];
}

int KeywordLangMatcher::GetLangScore(int8_t lang) const
{
  int const LANG_TIER_COUNT = static_cast<int>(m_languagePriorities.size());

  for (int i = 0; i < m_languagePriorities.size(); ++i)
    for (int j = 0; j < m_languagePriorities[i].size(); ++j)
      if (m_languagePriorities[i][j] == lang)
        return -i; // All languages in the same tier are equal.

  return -LANG_TIER_COUNT;
}

KeywordLangMatcher::ScoreT KeywordLangMatcher::Score(int8_t lang, string const & name) const
{
  return ScoreT(m_keywordMatcher.Score(name), GetLangScore(lang));
}

KeywordLangMatcher::ScoreT KeywordLangMatcher::Score(int8_t lang, StringT const & name) const
{
  return ScoreT(m_keywordMatcher.Score(name), GetLangScore(lang));
}

KeywordLangMatcher::ScoreT KeywordLangMatcher::Score(int8_t lang,
                                                     StringT const * tokens, size_t count) const
{
  return ScoreT(m_keywordMatcher.Score(tokens, count), GetLangScore(lang));
}

}  // namespace search
