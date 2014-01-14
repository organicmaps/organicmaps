#pragma once
#include "search_common.hpp"

#include "../base/string_utils.hpp"

#include "../std/string.hpp"
#include "../std/vector.hpp"

namespace search
{

class KeywordMatcher
{
public:
  typedef strings::UniString StringT;

  class ScoreT
  {
  public:
    ScoreT();
    bool operator < (ScoreT const & s) const;

  private:
    friend class KeywordMatcher;
    friend string DebugPrint(ScoreT const & score);

    bool IsQueryMatched() const { return m_bFullQueryMatched; }

    uint32_t m_sumTokenMatchDistance;
    uint32_t m_nameTokensMatched;
    uint32_t m_nameTokensLength;
    uint8_t m_numQueryTokensAndPrefixMatched;
    bool m_bFullQueryMatched : 1;
    bool m_bPrefixMatched : 1;
  };

  KeywordMatcher();

  void Clear();

  /// Internal copy of keywords is made.
  void SetKeywords(StringT const * keywords, size_t count, StringT const & prefix);

  /// @return Score of the name (greater is better).
  //@{
  ScoreT Score(string const & name) const;
  ScoreT Score(StringT const & name) const;
  ScoreT Score(StringT const * tokens, size_t count) const;
  //@}

  static bool IsQueryMatched(ScoreT const & score) { return score.IsQueryMatched(); }

private:
  vector<StringT> m_keywords;
  StringT m_prefix;
};

}  // namespace search
