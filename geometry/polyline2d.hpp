#pragma once

#include "geometry/parametrized_segment.hpp"
#include "geometry/point2d.hpp"
#include "geometry/point_with_altitude.hpp"
#include "geometry/rect2d.hpp"

#include "base/internal/message.hpp"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace m2
{
/// \returns a pair of minimum squared distance from |point| to the closest segment and
/// a zero-based closest segment index in points in range [|beginIt|, |endIt|).
template <typename It, typename T>
std::pair<double, uint32_t> CalcMinSquaredDistance(It beginIt, It endIt, m2::Point<T> const & point)
{
  CHECK_GREATER(std::distance(beginIt, endIt), 1, ());
  auto squaredClosestSegDist = std::numeric_limits<double>::max();

  auto i = beginIt;
  auto closestSeg = beginIt;
  for (auto j = i + 1; j != endIt; ++i, ++j)
  {
    m2::ParametrizedSegment<m2::Point<T>> seg(geometry::GetPoint(*i), geometry::GetPoint(*j));
    auto const squaredSegDist = seg.SquaredDistanceToPoint(point);
    if (squaredSegDist < squaredClosestSegDist)
    {
      closestSeg = i;
      squaredClosestSegDist = squaredSegDist;
    }
  }

  return std::make_pair(squaredClosestSegDist, static_cast<uint32_t>(std::distance(beginIt, closestSeg)));
}

template <typename T>
class Polyline
{
  std::vector<Point<T>> m_points;

public:
  using Container = std::vector<Point<T>>;
  using Iter = typename Container::const_iterator;

  Polyline() {}
  Polyline(std::initializer_list<Point<T>> const & points) : m_points(points)
  {
    ASSERT_GREATER(m_points.size(), 1, ());
  }

  explicit Polyline(std::vector<Point<T>> const & points) : m_points(points) { ASSERT_GREATER(m_points.size(), 1, ()); }

  explicit Polyline(std::vector<Point<T>> && points) : m_points(std::move(points))
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
      dist += m_points[i - 1].Length(m_points[i]);
    return dist;
  }

  double GetLength(size_t pointIndex) const
  {
    double dist = 0;
    for (size_t i = 0; i < std::min(pointIndex, m_points.size() - 1); ++i)
      dist += m_points[i].Length(m_points[i + 1]);
    return dist;
  }

  std::pair<double, uint32_t> CalcMinSquaredDistance(m2::Point<T> const & point) const
  {
    return m2::CalcMinSquaredDistance(m_points.begin(), m_points.end(), point);
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
  void Append(Polyline const & poly) { m_points.insert(m_points.end(), poly.m_points.cbegin(), poly.m_points.cend()); }

  template <class Iter>
  void Append(Iter beg, Iter end)
  {
    m_points.insert(m_points.end(), beg, end);
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

  std::vector<Point<T>> ExtractSegment(size_t segmentIndex, bool reversed) const
  {
    if (segmentIndex + 1 >= m_points.size())
      return std::vector<Point<T>>();

    return reversed ? std::vector<Point<T>>{m_points[segmentIndex + 1], m_points[segmentIndex]}
                    : std::vector<Point<T>>{m_points[segmentIndex], m_points[segmentIndex + 1]};
  }

  std::vector<Point<T>> ExtractSegment(size_t startPointIndex, size_t endPointIndex) const
  {
    if (startPointIndex > endPointIndex || startPointIndex >= m_points.size() || endPointIndex >= m_points.size())
      return std::vector<Point<T>>();

    std::vector<Point<T>> result(endPointIndex - startPointIndex + 1);
    for (size_t i = startPointIndex, j = 0; i <= endPointIndex; ++i, ++j)
      result[j] = m_points[i];
    return result;
  }

  std::vector<Point<T>> const & GetPoints() const { return m_points; }
  friend std::string DebugPrint(Polyline const & p) { return ::DebugPrint(p.m_points); }
};

using PolylineD = Polyline<double>;
}  // namespace m2
