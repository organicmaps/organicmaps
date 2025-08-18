#include "keyword_lang_matcher.hpp"

#include "indexer/search_delimiters.hpp"
#include "indexer/search_string_utils.hpp"

#include "base/assert.hpp"
#include "base/stl_helpers.hpp"

#include <algorithm>
#include <limits>
#include <sstream>

using namespace std;

namespace search
{
// KeywordLangMatcher::Score ----------------------------------------------------------------------
KeywordLangMatcher::Score::Score() : m_langScore(numeric_limits<int>::min()) {}

KeywordLangMatcher::Score::Score(KeywordMatcher::Score const & score, int langScore)
  : m_parentScore(score)
  , m_langScore(langScore)
{}

bool KeywordLangMatcher::Score::operator<(KeywordLangMatcher::Score const & score) const
{
  if (m_parentScore != score.m_parentScore)
    return m_parentScore < score.m_parentScore;

  if (m_langScore != score.m_langScore)
    return m_langScore < score.m_langScore;

  return m_parentScore.LessInTokensLength(score.m_parentScore);
}

bool KeywordLangMatcher::Score::operator<=(KeywordLangMatcher::Score const & score) const
{
  return !(score < *this);
}
// KeywordLangMatcher ------------------------------------------------------------------------------
KeywordLangMatcher::KeywordLangMatcher(size_t maxLanguageTiers) : m_languagePriorities(maxLanguageTiers)
{
  // Should we ever have this many tiers, the idea of storing a vector of vectors must be revised.
  ASSERT_LESS(maxLanguageTiers, 10, ());
}

void KeywordLangMatcher::SetLanguages(size_t tier, std::vector<int8_t> && languages)
{
  ASSERT_LESS(tier, m_languagePriorities.size(), ());
  m_languagePriorities[tier] = std::move(languages);
}

int KeywordLangMatcher::CalcLangScore(int8_t lang) const
{
  int const numTiers = static_cast<int>(m_languagePriorities.size());
  for (int i = 0; i < numTiers; ++i)
  {
    for (int8_t x : m_languagePriorities[i])
      if (x == lang)
        return -i;
  }

  return -numTiers;
}

KeywordLangMatcher::Score KeywordLangMatcher::CalcScore(int8_t lang, string_view name) const
{
  return Score(m_keywordMatcher.CalcScore(name), CalcLangScore(lang));
}

KeywordLangMatcher::Score KeywordLangMatcher::CalcScore(int8_t lang, strings::UniString const & name) const
{
  return Score(m_keywordMatcher.CalcScore(name), CalcLangScore(lang));
}

KeywordLangMatcher::Score KeywordLangMatcher::CalcScore(int8_t lang, strings::UniString const * tokens,
                                                        size_t count) const
{
  return Score(m_keywordMatcher.CalcScore(tokens, count), CalcLangScore(lang));
}

// Functions ---------------------------------------------------------------------------------------
string DebugPrint(KeywordLangMatcher::Score const & score)
{
  ostringstream ss;
  ss << "KLM::Score(" << DebugPrint(score.m_parentScore) << ", LS=" << score.m_langScore << ")";
  return ss.str();
}
}  // namespace search
