#pragma once
#include "../base/base.hpp"
#include "../base/buffer_vector.hpp"
#include "../base/string_utils.hpp"
#include "../std/string.hpp"

namespace search
{
namespace impl
{

typedef uint32_t (* StringMatchFn)(strings::UniChar const * sA, uint32_t sizeA,
                                   strings::UniChar const * sB, uint32_t sizeB,
                                   uint32_t maxCost);


// Matches keywords agains given names.
class KeywordMatcher
{
  strings::UniString * m_pKewords;
  strings::UniString const & m_prefix;
  uint32_t m_maxKeywordMatchCost, m_maxPrefixMatchCost;
  StringMatchFn m_keywordMatchFn, m_prefixMatchFn;
  buffer_vector<uint32_t, 8> m_minKeywordMatchCost;
  uint32_t m_minPrefixMatchCost;

public:
  KeywordMatcher(strings::UniString * pKeywords,
                 size_t keywordsCount,
                 strings::UniString const & prefix,
                 uint32_t maxKeywordMatchCost, uint32_t maxPrefixMatchCost,
                 StringMatchFn keywordMatchFn, StringMatchFn prefixMatchFn);

  void ProcessName(string const & name);

  // Useful for FeatureType.ForEachName(), calls ProcessName() and always returns true.
  bool operator () (int /*lang*/, string const & name)
  {
    ProcessName(name);
    return true;
  }

  // Get total feature match score.
  uint32_t GetMatchScore() const;

  // Get prefix match score.
  uint32_t GetPrefixMatchScore() const { return m_minPrefixMatchCost; }

  // Get match score for each keyword.
  uint32_t const * GetKeywordMatchScores() const { return &m_minKeywordMatchCost[0]; }
};

}  // namespace search::impl
}  // namespace search
