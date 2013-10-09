#pragma once

#include "point2d.hpp"
#include "rect2d.hpp"

namespace m2
{

template <typename T>
class PolylineT
{
public:
  PolylineT() {}
  explicit PolylineT(vector<Point<T> > const & points) : m_points(points)
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  // let it be public for now
  vector<Point<T> > m_points;

  double GetLength() const
  {
    // @todo add cached value and lazy init
    double dist = 0;
    for (size_t i = 1; i < m_points.size(); ++i)
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

  void Clear() { m_points.clear(); }
  void Add(Point<T> const & pt) { m_points.push_back(pt); }
  void Swap(PolylineT & rhs)
  {
    m_points.swap(rhs.m_points);
  }
  size_t GetSize() const { return m_points.size(); }
};

typedef PolylineT<double> PolylineD;

}
