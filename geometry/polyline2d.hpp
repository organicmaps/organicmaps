#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"

namespace m2 {

template <typename T>
class Polyline2d
{
public:
  Polyline2d() {}
  explicit Polyline2d(vector<Point<T> > points): m_points(points) {}

  // let it be public for now
  vector<Point<T> > m_points;

  T GetLength() const
  {
    // @todo add cached value and lazy init
    T dist = 0;
    for (int i = 1; i < m_points.size(); ++i)
      dist += m_points[i-1].Length(m_points[i]);
    return dist;
  }

  Rect<T> GetLimitRect() const
  {
    // @todo add cached value and lazy init
    m2::Rect<T> rect;
    for (size_t i = 0; i < m_points.size(); ++i)
      rect.Add(m_points[i]);
    return rect;
  }
};
}
