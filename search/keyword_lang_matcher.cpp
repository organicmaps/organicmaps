#include "keyword_lang_matcher.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_add.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

using namespace std;

namespace search
{
// KeywordLangMatcher::ScoreT ----------------------------------------------------------------------
KeywordLangMatcher::ScoreT::ScoreT() : m_langScore(numeric_limits<int>::min())
{
}

KeywordLangMatcher::ScoreT::ScoreT(KeywordMatcher::ScoreT const & score, int langScore)
  : m_parentScore(score), m_langScore(langScore)
{
}

bool KeywordLangMatcher::ScoreT::operator<(KeywordLangMatcher::ScoreT const & score) const
{
  if (m_parentScore != score.m_parentScore)
    return m_parentScore < score.m_parentScore;

  if (m_langScore != score.m_langScore)
    return m_langScore < score.m_langScore;

  return m_parentScore.LessInTokensLength(score.m_parentScore);
}

// KeywordLangMatcher ------------------------------------------------------------------------------
KeywordLangMatcher::KeywordLangMatcher(size_t maxLanguageTiers)
  : m_languagePriorities(maxLanguageTiers)
{
  // Should we ever have this many tiers, the idea of storing a vector of vectors must be revised.
  ASSERT_LESS(maxLanguageTiers, 10, ());
}

void KeywordLangMatcher::SetLanguages(size_t tier, std::vector<int8_t> && languages)
{
  ASSERT_LESS(tier, m_languagePriorities.size(), ());
  m_languagePriorities[tier] = move(languages);
}

int KeywordLangMatcher::GetLangScore(int8_t lang) const
{
  int const numTiers = static_cast<int>(m_languagePriorities.size());
  for (int i = 0; i < numTiers; ++i)
  {
    for (int8_t x : m_languagePriorities[i])
    {
      if (x == lang)
        return -i;
    }
  }

  return -numTiers;
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

// Functions ---------------------------------------------------------------------------------------
string DebugPrint(KeywordLangMatcher::ScoreT const & score)
{
  ostringstream ss;
  ss << "KLM::ScoreT(" << DebugPrint(score.m_parentScore) << ", LS=" << score.m_langScore << ")";
  return ss.str();
}
}  // namespace search
