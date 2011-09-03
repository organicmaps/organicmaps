#pragma once
#include "search_common.hpp"
#include "../base/assert.hpp"
#include "../base/buffer_vector.hpp"
#include "../base/string_utils.hpp"
#include "../std/string.hpp"

namespace search
{

class KeywordMatcher
{
public:
  enum { MAX_SCORE = MAX_TOKENS };

  KeywordMatcher(strings::UniString const * const * pKeywords, int keywordCount,
                 strings::UniString const * pPrefix);


  // Returns penalty (which is less than MAX_SCORE) if name matched, or MAX_SCORE otherwise.
  uint32_t Score(string const & name) const;
  uint32_t Score(strings::UniString const & name) const;
  uint32_t Score(strings::UniString const * tokens, int tokenCount) const;

private:
  strings::UniString const * const * m_pKeywords;
  int m_keywordCount;
  strings::UniString const * m_pPrefix;
};

}  // namespace search
