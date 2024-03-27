#pragma once

#include "search/common.hpp"

#include "base/string_utils.hpp"

#include <string>
#include <vector>

namespace search
{
class KeywordMatcher
{
public:
  class Score
  {
  public:
    Score();

    // *NOTE* m_nameTokensLength is usually used as a late stage tiebreaker
    // and does not take part in the operators.
    bool operator<(Score const & s) const;
    bool operator==(Score const & s) const;
    bool operator!=(Score const & s) const { return !(*this == s); }

    bool LessInTokensLength(Score const & s) const;

    bool IsQueryMatched() const { return m_fullQueryMatched; }

  private:
    friend class KeywordMatcher;
    friend std::string DebugPrint(Score const & score);

    uint32_t m_sumTokenMatchDistance;
    uint32_t m_nameTokensMatched;
    uint32_t m_nameTokensLength;
    uint8_t m_numQueryTokensAndPrefixMatched;
    bool m_fullQueryMatched : 1;
    bool m_prefixMatched : 1;
  };

  KeywordMatcher();

  void Clear();

  /// Internal copy of keywords is made.
  void SetKeywords(QueryString const & query);

  /// @return Score of the name (greater is better).
  //@{
  Score CalcScore(std::string_view name) const;
  Score CalcScore(strings::UniString const & name) const;
  Score CalcScore(strings::UniString const * tokens, size_t count) const;
  //@}

private:
  std::vector<strings::UniString> m_keywords;
  strings::UniString m_prefix;
};
}  // namespace search
