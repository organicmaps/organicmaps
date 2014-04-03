#pragma once

#include "../geometry/point2d.hpp"
#include "../geometry/rect2d.hpp"


namespace search
{

double PointDistance(m2::PointD const & a, m2::PointD const & b);

uint8_t ViewportDistance(m2::RectD const & viewport, m2::PointD const & p);

}
