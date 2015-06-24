#pragma once

#include "base/macros.hpp"
#include "base/assert.hpp"

#include "std/string.hpp"
#include "std/cstring.hpp"


namespace strings { class UniString; }

namespace feature
{
  static int const g_arrWorldScales[] = { 3, 5, 7, 9 };    // 9 = scales::GetUpperWorldScale()
  static int const g_arrCountryScales[] = { 10, 12, 14, 17 };  // 17 = scales::GetUpperScale()

  inline string GetTagForIndex(char const * prefix, int ind)
  {
    string str;
    str.reserve(strlen(prefix) + 1);
    str = prefix;

    static char const arrChar[] = { '0', '1', '2', '3' };
    static_assert(ARRAY_SIZE(arrChar) >= ARRAY_SIZE(g_arrWorldScales), "");
    static_assert(ARRAY_SIZE(arrChar) >= ARRAY_SIZE(g_arrCountryScales), "");
    ASSERT(ind >= 0 && ind < ARRAY_SIZE(arrChar), (ind));

    str += arrChar[ind];
    return str;
  }

  bool IsNumber(strings::UniString const & s);

  bool IsHouseNumber(string const & s);
  bool IsHouseNumberDeepCheck(strings::UniString const & s);
}
