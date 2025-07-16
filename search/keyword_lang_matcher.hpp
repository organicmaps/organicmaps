#pragma once

#include "search/keyword_matcher.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <vector>

namespace search
{
class KeywordLangMatcher
{
public:
  class Score
  {
  public:
    Score();
    bool operator<(Score const & s) const;
    bool operator<=(Score const & s) const;

  private:
    friend class KeywordLangMatcher;
    friend std::string DebugPrint(Score const & score);

    Score(KeywordMatcher::Score const & score, int langScore);

    KeywordMatcher::Score m_parentScore;
    int m_langScore;
  };

  // Constructs a matcher that supports up to |maxLanguageTiers| tiers.
  // All languages in the same tier are considered equal.
  // The lower the tier is, the more important the languages in it are.
  explicit KeywordLangMatcher(size_t maxLanguageTiers);

  // Defines the languages in the |tier| to be exactly |languages|.
  void SetLanguages(size_t const tier, std::vector<int8_t> && languages);

  // Calls |fn| on every language in every tier. Does not make a distinction
  // between languages in different tiers.
  template <typename Fn>
  void ForEachLanguage(Fn && fn) const
  {
    for (auto const & langs : m_languagePriorities)
      for (int8_t lang : langs)
        fn(lang);
  }

  // Store references to keywords from source array of strings.
  inline void SetKeywords(QueryString const & query) { m_keywordMatcher.SetKeywords(query); }

  // Returns the Score of the name (greater is better).
  Score CalcScore(int8_t lang, std::string_view name) const;
  Score CalcScore(int8_t lang, strings::UniString const & name) const;
  Score CalcScore(int8_t lang, strings::UniString const * tokens, size_t count) const;

private:
  int CalcLangScore(int8_t lang) const;

  std::vector<std::vector<int8_t>> m_languagePriorities;
  KeywordMatcher m_keywordMatcher;
};
}  // namespace search
