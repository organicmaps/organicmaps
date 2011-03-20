#pragma once

#include "point_to_int64.hpp"

#include "../geometry/point2d.hpp"


namespace feature
{
  namespace pts
  {
    inline int64_t FromPoint(m2::PointD const & p)
    {
      return PointToInt64(p.x, p.y);
    }

    inline m2::PointD ToPoint(int64_t i)
    {
      CoordPointT const pt = Int64ToPoint(i);
      return m2::PointD(pt.first, pt.second);
    }
  }


  static int g_arrWorldScales[] = { 2, 4, 5, 6 };       // 6 = scales::GetUpperWorldScale()
  static int g_arrCountryScales[] = { 7, 10, 14, 17 };  // 17 = scales::GetUpperScale()

  inline string GetTagForIndex(char const * prefix, int ind)
  {
    string str;
    str.reserve(strlen(prefix) + 1);
    str = prefix;

    static char arrChar[] = { '0', '1', '2', '3' };
    STATIC_ASSERT ( ARRAY_SIZE(arrChar) == ARRAY_SIZE(g_arrWorldScales) );
    STATIC_ASSERT ( ARRAY_SIZE(arrChar) == ARRAY_SIZE(g_arrCountryScales) );
    ASSERT ( ind >= 0 && ind < ARRAY_SIZE(arrChar), (ind) );

    str += arrChar[ind];
    return str;
  }
}
