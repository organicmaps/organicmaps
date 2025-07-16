#pragma once

#include "indexer/search_delimiters.hpp"

#include "search/common.hpp"
#include "search/result.hpp"

#include "base/string_utils.hpp"

#include <cstdint>
#include <string>
#include <utility>

namespace search
{
template <typename LowTokensIter, typename F>
void SearchStringTokensIntersectionRanges(std::string const & s, LowTokensIter itLowBeg, LowTokensIter itLowEnd, F && f)
{
  // split input query by tokens and prefix
  search::Delimiters delimsTest;
  size_t pos = 0;

  strings::UniString const str = strings::MakeUniString(s);
  size_t const strLen = str.size();
  while (pos < strLen)
  {
    // skip delimeters
    while (pos < strLen && delimsTest(str[pos]))
      ++pos;

    size_t const beg = pos;

    // find token
    while (pos < strLen && !delimsTest(str[pos]))
      ++pos;

    strings::UniString subStr;
    subStr.assign(str.begin() + beg, str.begin() + pos);
    size_t maxCount = 0;
    std::pair<uint16_t, uint16_t> result(0, 0);

    for (auto itLow = itLowBeg; itLow != itLowEnd; ++itLow)
    {
      size_t const cnt = strings::CountNormLowerSymbols(subStr, *itLow);

      if (cnt > maxCount)
      {
        maxCount = cnt;
        result.first = beg;
        result.second = cnt;
      }
    }

    if (result.second != 0)
      f(result);
  }
}

// Adds to |res| the ranges that match the query tokens and, therefore, should be highlighted.
// The query is passed in |tokens| and |prefix|.
void HighlightResult(QueryTokens const & tokens, strings::UniString const & prefix, Result & res);
}  // namespace search
