#pragma once

#include "point2d.hpp"

namespace m2
{

template <class T>
double GetTriangleArea(Point<T> const & p1, Point<T> const & p2, Point<T> const & p3)
{
  return 0.5 * fabs((p2.x - p1.x)*(p3.y - p1.y) - (p3.x - p1.x)*(p2.y - p1.y));
}

}
