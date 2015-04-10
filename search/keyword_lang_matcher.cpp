#include "search/keyword_lang_matcher.hpp"

#include "indexer/search_string_utils.hpp"
#include "indexer/search_delimiters.hpp"

#include "base/stl_add.hpp"

#include "std/algorithm.hpp"


namespace search
{

KeywordLangMatcher::ScoreT::ScoreT() : m_langScore(numeric_limits<int>::min())
{
}

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

  return m_parentScore.LessInTokensLength(score.m_parentScore);
}

void KeywordLangMatcher::SetLanguages(vector<vector<int8_t> > const & languagePriorities)
{
  m_languagePriorities = languagePriorities;
}

void KeywordLangMatcher::SetLanguage(pair<int, int> const & ind, int8_t lang)
{
  m_languagePriorities[ind.first][ind.second] = lang;
}

int8_t KeywordLangMatcher::GetLanguage(pair<int, int> const & ind) const
{
  return m_languagePriorities[ind.first][ind.second];
}

int KeywordLangMatcher::GetLangScore(int8_t lang) const
{
  int const prioritiesTiersCount = static_cast<int>(m_languagePriorities.size());
  for (int i = 0; i < prioritiesTiersCount; ++i)
    for (int j = 0; j < m_languagePriorities[i].size(); ++j)
      if (m_languagePriorities[i][j] == lang)
        return -i; // All languages in the same tier are equal.

  return -prioritiesTiersCount;
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

string DebugPrint(KeywordLangMatcher::ScoreT const & score)
{
  ostringstream ss;
  ss << "KLM::ScoreT(" << DebugPrint(score.m_parentScore) << ", LS=" << score.m_langScore << ")";
  return ss.str();
}

}  // namespace search
