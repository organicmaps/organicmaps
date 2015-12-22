#pragma once

#include "geometry/point2d.hpp"

class FeatureType;

namespace feature
{

m2::PointD GetCenter(FeatureType const & f, int scale);
m2::PointD GetCenter(FeatureType const & f);

double GetMinDistance(FeatureType const & ft, m2::PointD const & pt, int scale);
double GetMinDistance(FeatureType const & ft, m2::PointD const & pt);

}
