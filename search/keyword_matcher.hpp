#pragma once
#include "search_common.hpp"

#include "../base/string_utils.hpp"

#include "../std/string.hpp"


namespace search
{

class KeywordMatcher
{
public:
  enum { MAX_SCORE = MAX_TOKENS };

  KeywordMatcher(strings::UniString const * const * pKeywords, size_t keywordCount,
                 strings::UniString const * pPrefix);
  KeywordMatcher(strings::UniString const * keywords, size_t keywordCount,
                 strings::UniString const * pPrefix);
  ~KeywordMatcher();


  // Returns penalty (which is less than MAX_SCORE) if name matched, or MAX_SCORE otherwise.
  uint32_t Score(string const & name) const;
  uint32_t Score(strings::UniString const & name) const;
  uint32_t Score(strings::UniString const * tokens, int tokenCount) const;

private:
  void Initialize();

  strings::UniString const * const * m_pKeywords;
  size_t m_keywordCount;
  strings::UniString const * m_pPrefix;
  bool m_bOwnKeywords;
};

}  // namespace search
