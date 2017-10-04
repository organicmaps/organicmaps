#pragma once

#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"
#include "geometry/distance.hpp"

#include "base/internal/message.hpp"

#include "std/initializer_list.hpp"
#include "std/vector.hpp"

namespace m2
{
template <typename T>
class Polyline
{
  vector<Point<T> > m_points;

public:
  using Container = vector<Point<T>>;
  using Iter = typename Container::const_iterator;

  Polyline() {}
  Polyline(initializer_list<Point<T>> const & points) : m_points(points)
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  explicit Polyline(vector<Point<T>> const & points) : m_points(points)
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  explicit Polyline(vector<Point<T>> && points) : m_points(move(points))
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  template <class Iter>
  Polyline(Iter beg, Iter end) : m_points(beg, end)
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  double GetLength() const
  {
    // @todo add cached value and lazy init
    double dist = 0;
    for (size_t i = 1; i < m_points.size(); ++i)
      dist += m_points[i-1].Length(m_points[i]);
    return dist;
  }

  double CalcMinSquaredDistance(m2::Point<T> const & point) const
  {
    double res = numeric_limits<double>::max();
    m2::DistanceToLineSquare<m2::Point<T>> d;

    Iter i = Begin();
    for (Iter j = i + 1; j != End(); ++i, ++j)
    {
      d.SetBounds(*i, *j);
      res = min(res, d(point));
    }

    return res;
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
  void Append(Polyline const & poly)
  {
    m_points.insert(m_points.end(), poly.m_points.cbegin(), poly.m_points.cend());
  }

  void PopBack()
  {
    ASSERT(!m_points.empty(), ());
    m_points.pop_back();
  }

  void Swap(Polyline & rhs) { m_points.swap(rhs.m_points); }
  size_t GetSize() const { return m_points.size(); }
  bool operator==(Polyline const & rhs) const { return m_points == rhs.m_points; }
  Iter Begin() const { return m_points.begin(); }
  Iter End() const { return m_points.end(); }
  Point<T> const & Front() const { return m_points.front(); }
  Point<T> const & Back() const { return m_points.back(); }

  Point<T> const & GetPoint(size_t idx) const
  {
    ASSERT_LESS(idx, m_points.size(), ());
    return m_points[idx];
  }

  Point<T> GetPointByDistance(T distance) const
  {
    if (distance < 0)
      return m_points.front();

    T dist = 0;
    for (size_t i = 1; i < m_points.size(); ++i)
    {
      T const oldDist = dist;
      dist += m_points[i - 1].Length(m_points[i]);
      if (distance <= dist)
      {
        T const t = (distance - oldDist) / (dist - oldDist);
        return m_points[i - 1] * (1 - t) + m_points[i] * t;
      }
    }

    return m_points.back();
  }

  vector<Point<T>> ExtractSegment(size_t segmentIndex, bool reversed) const
  {
    if (segmentIndex + 1 >= m_points.size())
      return vector<Point<T>>();

    return reversed ? vector<Point<T>>{m_points[segmentIndex + 1], m_points[segmentIndex]} :
           vector<Point<T>>{m_points[segmentIndex], m_points[segmentIndex + 1]};
  }

  vector<Point<T>> ExtractSegment(size_t startPointIndex, size_t endPointIndex) const
  {
    if (startPointIndex > endPointIndex ||
        startPointIndex + 1 > m_points.size() ||
        endPointIndex + 1 > m_points.size())
    {
      return vector<Point<T>>();
    }

    vector<Point<T>> result;
    result.reserve(endPointIndex - startPointIndex + 1);
    for (size_t i = startPointIndex; i <= endPointIndex; ++i)
      result.push_back(m_points[i]);
    return result;
  }

  vector<Point<T> > const & GetPoints() const { return m_points; }
  friend string DebugPrint(Polyline const & p) { return ::DebugPrint(p.m_points); }
};

using PolylineD = Polyline<double>;
}  // namespace m2
