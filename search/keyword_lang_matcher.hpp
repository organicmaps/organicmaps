#pragma once
#include "search/keyword_matcher.hpp"

#include "std/utility.hpp"
#include "std/vector.hpp"

namespace search
{

class KeywordLangMatcher
{
public:
  class ScoreT
  {
  public:
    ScoreT();
    bool operator < (ScoreT const & s) const;

  private:
    friend class KeywordLangMatcher;
    friend string DebugPrint(ScoreT const & score);

    ScoreT(KeywordMatcher::ScoreT const & score, int langScore);

    KeywordMatcher::ScoreT m_parentScore;
    int m_langScore;
  };

private:
  typedef KeywordMatcher::StringT StringT;

public:
  /// @param[in] languagePriorities Should match the constants above (checked by assertions).
  void SetLanguages(vector<vector<int8_t> > const & languagePriorities);
  void SetLanguage(pair<int, int> const & ind, int8_t lang);
  int8_t GetLanguage(pair<int, int> const & ind) const;

  /// Store references to keywords from source array of strings.
  inline void SetKeywords(StringT const * keywords, size_t count, StringT const & prefix)
  {
    m_keywordMatcher.SetKeywords(keywords, count, prefix);
  }

  /// @return Score of the name (greater is better).
  //@{
  ScoreT Score(int8_t lang, string const & name) const;
  ScoreT Score(int8_t lang, StringT const & name) const;
  ScoreT Score(int8_t lang, StringT const * tokens, size_t count) const;
  //@}

private:
  int GetLangScore(int8_t lang) const;

  vector<vector<int8_t> > m_languagePriorities;
  KeywordMatcher m_keywordMatcher;
};

}  // namespace search
