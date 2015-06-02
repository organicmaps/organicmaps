#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"


namespace search
{

// Distance between 2 mercator points in meters.
double PointDistance(m2::PointD const & a, m2::PointD const & b);
// Test for equal rects with epsilon in meters.
bool IsEqualMercator(m2::RectD const & r1, m2::RectD const & r2, double epsMeters);
// Get inflated viewport rect for search query.
bool GetInflatedViewport(m2::RectD & viewport);
// Get scale level to make geometry index query for current viewport.
int GetQueryIndexScale(m2::RectD const & viewport);

}
