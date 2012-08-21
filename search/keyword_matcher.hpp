#pragma once
#include "search_common.hpp"

#include "../base/string_utils.hpp"

#include "../std/string.hpp"


namespace search
{

class KeywordMatcher
{
public:
  enum { MAX_SCORE = MAX_TOKENS * MAX_TOKENS };
  typedef strings::UniString StringT;

  KeywordMatcher() : m_prefix(0) {}

  inline void Clear()
  {
    m_keywords.clear();
    m_prefix = 0;
  }

  /// Store references to keywords from source array of strings.
  void SetKeywords(StringT const * keywords, size_t count, StringT const * prefix);

  /// @return penalty of string (less is better).
  //@{
  uint32_t Score(string const & name) const;
  uint32_t Score(StringT const & name) const;
  uint32_t Score(StringT const * tokens, size_t count) const;
  //@}

private:
  buffer_vector<StringT const *, 10> m_keywords;
  StringT const * m_prefix;
};

}  // namespace search
