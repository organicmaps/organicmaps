#pragma once

#include "geometry/point2d.hpp"

class FeatureType;

namespace feature
{
m2::PointD GetCenter(FeatureType & f, int scale);
m2::PointD GetCenter(FeatureType & f);

double GetMinDistanceMeters(FeatureType & ft, m2::PointD const & pt, int scale);
double GetMinDistanceMeters(FeatureType & ft, m2::PointD const & pt);

}  // namespace feature
