#pragma once

#include "base/macros.hpp"
#include "base/assert.hpp"

#include <cstring>
#include <string>

namespace strings { class UniString; }

namespace feature
{
  static int const g_arrWorldScales[] = { 3, 5, 7, 9 };    // 9 = scales::GetUpperWorldScale()
  static int const g_arrCountryScales[] = { 10, 12, 14, 17 };  // 17 = scales::GetUpperScale()

  inline std::string GetTagForIndex(char const * prefix, size_t ind)
  {
    std::string str;
    str.reserve(strlen(prefix) + 1);
    str = prefix;

    static char const arrChar[] = { '0', '1', '2', '3' };
    static_assert(ARRAY_SIZE(arrChar) >= ARRAY_SIZE(g_arrWorldScales), "");
    static_assert(ARRAY_SIZE(arrChar) >= ARRAY_SIZE(g_arrCountryScales), "");
    ASSERT(ind >= 0 && ind < ARRAY_SIZE(arrChar), (ind));

    str += arrChar[ind];
    return str;
  }

  bool IsDigit(int c);
  bool IsNumber(strings::UniString const & s);

  bool IsHouseNumber(std::string const & s);
  bool IsHouseNumber(strings::UniString const & s);
  bool IsHouseNumberDeepCheck(strings::UniString const & s);

  uint8_t PopulationToRank(uint64_t p);
  uint64_t RankToPopulation(uint8_t r);
} // namespace feature
