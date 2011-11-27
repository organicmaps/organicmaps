#pragma once
#include "keyword_matcher.hpp"
#include "../base/base.hpp"
#include "../std/vector.hpp"

namespace search
{

class LangKeywordsScorer
{
public:
  enum { NUM_LANG_PRIORITY_TIERS = 3 };
  enum { MAX_LANGS_IN_TIER = 3 };
  enum { MAX_SCORE = KeywordMatcher::MAX_SCORE
         * (NUM_LANG_PRIORITY_TIERS + 1) * (MAX_LANGS_IN_TIER + 1) };

  explicit LangKeywordsScorer(vector<vector<int8_t> > const & languagePriorities,
                              strings::UniString const * keywords, size_t keywordCount,
                              strings::UniString const * pPrefix);

  uint32_t Score(int8_t lang, string const & name) const;
  uint32_t Score(int8_t lang, strings::UniString const & name) const;
  uint32_t Score(int8_t lang, strings::UniString const * tokens, int tokenCount) const;

private:
  vector<vector<int8_t> > m_languagePriorities;
  KeywordMatcher m_keywordMatcher;
};

}  // namespace search
