#pragma once

#include "geometry/point2d.hpp"

class FeatureType;

namespace feature
{

m2::PointD GetCenter(FeatureType const & f, int scale);
m2::PointD GetCenter(FeatureType const & f);

double GetMinDistanceMeters(FeatureType const & ft, m2::PointD const & pt, int scale);
double GetMinDistanceMeters(FeatureType const & ft, m2::PointD const & pt);

}  // namespace feature
