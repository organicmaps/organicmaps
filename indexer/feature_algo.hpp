#pragma once

#include "geometry/point2d.hpp"

class FeatureType;

namespace feature
{

m2::PointD GetCenter(FeatureType const & f, int scale);
m2::PointD GetCenter(FeatureType const & f);

}
