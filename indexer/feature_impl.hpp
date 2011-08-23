#pragma once

#include "point_to_int64.hpp"

#include "../geometry/point2d.hpp"


namespace feature
{
  namespace pts
  {
    inline int64_t FromPoint(m2::PointD const & p, uint32_t coordBits)
    {
      return PointToInt64(p.x, p.y, coordBits);
    }

    inline m2::PointD ToPoint(int64_t i, uint32_t coordBits)
    {
      CoordPointT const pt = Int64ToPoint(i, coordBits);
      return m2::PointD(pt.first, pt.second);
    }
  }


  static int g_arrWorldScales[] = { 2, 5, 7, 9 };       // 9 = scales::GetUpperWorldScale()
  static int g_arrCountryScales[] = { 10, 12, 14, 17 };  // 17 = scales::GetUpperScale()

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

  template <class TCont>
  void CalcRect(TCont const & points, m2::RectD & rect)
  {
    for (size_t i = 0; i < points.size(); ++i)
      rect.Add(points[i]);
  }
}
