#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

class FeatureType;

namespace feature
{
m2::PointD GetCenter(FeatureType & f, int scale);
m2::PointD GetCenter(FeatureType & f);

double GetMinDistanceMeters(FeatureType & ft, m2::PointD const & pt, int scale);
double GetMinDistanceMeters(FeatureType & ft, m2::PointD const & pt);

double CalcArea(FeatureType & ft);

template <class Iter>
void CalcRect(Iter b, Iter e, m2::RectD & rect)
{
  while (b != e)
    rect.Add(*b++);
}

template <class Cont>
void CalcRect(Cont const & points, m2::RectD & rect)
{
  CalcRect(points.begin(), points.end(), rect);
}
}  // namespace feature
