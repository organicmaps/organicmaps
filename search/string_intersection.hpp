#pragma once

#include "indexer/search_delimiters.hpp"

#include "std/utility.hpp"

namespace search
{
template <typename LowTokensIterType, typename F>
void SearchStringTokensIntersectionRanges(string const & s, LowTokensIterType itLowBeg,
                                          LowTokensIterType itLowEnd, F f)
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
    pair<uint16_t, uint16_t> result(0, 0);

    for (LowTokensIterType itLow = itLowBeg; itLow != itLowEnd; ++itLow)
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
}
