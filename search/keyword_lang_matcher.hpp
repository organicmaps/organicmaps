#pragma once

#include "search/keyword_matcher.hpp"

#include <array>
#include <utility>
#include <vector>

namespace search
{
class KeywordLangMatcher
{
public:
  using StringT = KeywordMatcher::StringT;

  class ScoreT
  {
  public:
    ScoreT();
    bool operator<(ScoreT const & s) const;

  private:
    friend class KeywordLangMatcher;
    friend string DebugPrint(ScoreT const & score);

    ScoreT(KeywordMatcher::ScoreT const & score, int langScore);

    KeywordMatcher::ScoreT m_parentScore;
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
    {
      for (int8_t lang : langs)
        fn(lang);
    }
  }

  // Store references to keywords from source array of strings.
  inline void SetKeywords(StringT const * keywords, size_t count, StringT const & prefix)
  {
    m_keywordMatcher.SetKeywords(keywords, count, prefix);
  }

  // Returns the Score of the name (greater is better).
  ScoreT Score(int8_t lang, string const & name) const;
  ScoreT Score(int8_t lang, StringT const & name) const;
  ScoreT Score(int8_t lang, StringT const * tokens, size_t count) const;

private:
  int GetLangScore(int8_t lang) const;

  std::vector<std::vector<int8_t>> m_languagePriorities;
  KeywordMatcher m_keywordMatcher;
};
}  // namespace search
