#pragma once

#include "../geometry/point2d.hpp"


namespace feature
{
  static int g_arrWorldScales[] = { 5, 7, 9 };       // 9 = scales::GetUpperWorldScale()
  static int g_arrCountryScales[] = { 12, 15, 17 };  // 17 = scales::GetUpperScale()

  inline string GetTagForIndex(char const * prefix, int ind)
  {
    string str;
    str.reserve(strlen(prefix) + 1);
    str = prefix;

    static char arrChar[] = { '0', '1', '2' };
    STATIC_ASSERT ( ARRAY_SIZE(arrChar) == ARRAY_SIZE(g_arrWorldScales) );
    STATIC_ASSERT ( ARRAY_SIZE(arrChar) == ARRAY_SIZE(g_arrCountryScales) );
    ASSERT ( ind >= 0 && ind < ARRAY_SIZE(arrChar), (ind) );

    str += arrChar[ind];
    return str;
  }
}
