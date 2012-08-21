#pragma once
#include "keyword_matcher.hpp"

#include "../std/vector.hpp"


namespace search
{

class LangKeywordsScorer
{
  enum { NUM_LANG_PRIORITY_TIERS = 4 };
  enum { MAX_LANGS_IN_TIER = 2 };

  typedef KeywordMatcher::StringT StringT;

public:
  /// @param[in] languagePriorities Should match the constants above (checked by assertions).
  void SetLanguages(vector<vector<int8_t> > const & languagePriorities);
  void SetLanguage(pair<int, int> const & ind, int8_t lang);
  int8_t GetLanguage(pair<int, int> const & ind) const;

  /// Store references to keywords from source array of strings.
  inline void SetKeywords(StringT const * keywords, size_t count, StringT const * prefix)
  {
    m_keywordMatcher.SetKeywords(keywords, count, prefix);
  }

  uint32_t Score(int8_t lang, string const & name) const;
  uint32_t Score(int8_t lang, StringT const & name) const;
  uint32_t Score(int8_t lang, StringT const * tokens, size_t count) const;

private:
  bool AssertIndex(pair<int, int> const & ind) const;

  vector<vector<int8_t> > m_languagePriorities;
  KeywordMatcher m_keywordMatcher;
};

}  // namespace search
