#pragma once

#include "geometry/distance.hpp"
#include "geometry/point2d.hpp"
#include "geometry/rect2d.hpp"

#include "base/math.hpp"

#include <algorithm>
#include <type_traits>
#include <utility>
#include <vector>

namespace m2
{
namespace detail
{
struct DefEqualFloat
{
  // 1e-9 is two orders of magnitude more accurate than our OSM source data.
  static double constexpr kPrecision = 1e-9;

  template <typename Point>
  bool EqualPoints(Point const & p1, Point const & p2) const
  {
    static_assert(std::is_floating_point<typename Point::value_type>::value, "");

    return my::AlmostEqualAbs(p1.x, p2.x, static_cast<typename Point::value_type>(kPrecision)) &&
           my::AlmostEqualAbs(p1.y, p2.y, static_cast<typename Point::value_type>(kPrecision));
  }

  template <typename TCoord>
  bool EqualZeroSquarePrecision(TCoord val) const
  {
    static_assert(std::is_floating_point<TCoord>::value, "");

    return my::AlmostEqualAbs(val, 0.0, kPrecision * kPrecision);
  }
  // Determines if value of a val lays between a p1 and a p2 values with some precision.
  bool IsAlmostBetween(double val, double p1, double p2) const
  {
    return (val >= p1 - kPrecision && val <= p2 + kPrecision) ||
           (val <= p1 + kPrecision && val >= p2 - kPrecision);
  }
};

struct DefEqualInt
{
  template <typename Point>
  bool EqualPoints(Point const & p1, Point const & p2) const
  {
    return p1 == p2;
  }

  template <typename TCoord>
  bool EqualZeroSquarePrecision(TCoord val) const
  {
    return val == 0;
  }

  bool IsAlmostBetween(double val, double left, double right) const
  {
    return (val >= left && val <= right) || (val <= left && val >= right);
  }
};

template <int floating>
struct TraitsType;

template <>
struct TraitsType<1>
{
  typedef DefEqualFloat EqualType;
  typedef double BigType;
};

template <>
struct TraitsType<0>
{
  typedef DefEqualInt EqualType;
  typedef int64_t BigType;
};
}  // namespace detail

template <typename Point>
class Region
{
public:
  using ValueT = Point;
  using CoordT = typename Point::value_type;
  using ContainerT = std::vector<Point>;
  using TraitsT = detail::TraitsType<std::is_floating_point<CoordT>::value>;

  /// @name Needed for boost region concept.
  //@{
  using IteratorT = typename ContainerT::const_iterator;
  IteratorT Begin() const { return m_points.begin(); }
  IteratorT End() const { return m_points.end(); }
  size_t Size() const { return m_points.size(); }
  //@}

  Region() = default;

  template <typename Points>
  explicit Region(Points && points) : m_points(std::forward<Points>(points))
  {
    CalcLimitRect();
  }

  template <typename IterT>
  Region(IterT first, IterT last) : m_points(first, last)
  {
    CalcLimitRect();
  }

  template <typename IterT>
  void Assign(IterT first, IterT last)
  {
    m_points.assign(first, last);
    CalcLimitRect();
  }

  template <typename IterT, typename Fn>
  void AssignEx(IterT first, IterT last, Fn fn)
  {
    m_points.reserve(distance(first, last));

    while (first != last)
      m_points.push_back(fn(*first++));

    CalcLimitRect();
  }

  void AddPoint(Point const & pt)
  {
    m_points.push_back(pt);
    m_rect.Add(pt);
  }

  template <typename TFunctor>
  void ForEachPoint(TFunctor toDo) const
  {
    for_each(m_points.begin(), m_points.end(), toDo);
  }

  m2::Rect<CoordT> const & GetRect() const { return m_rect; }
  size_t GetPointsCount() const { return m_points.size(); }
  bool IsValid() const { return GetPointsCount() > 2; }

  void Swap(Region<Point> & rhs)
  {
    m_points.swap(rhs.m_points);
    std::swap(m_rect, rhs.m_rect);
  }

  ContainerT const & Data() const { return m_points; }

  template <typename TEqualF>
  static bool IsIntersect(CoordT const & x11, CoordT const & y11, CoordT const & x12,
                          CoordT const & y12, CoordT const & x21, CoordT const & y21,
                          CoordT const & x22, CoordT const & y22, TEqualF equalF, Point & pt)
  {
    double const divider = ((y12 - y11) * (x22 - x21) - (x12 - x11) * (y22 - y21));
    if (equalF.EqualZeroSquarePrecision(divider))
      return false;
    double v = ((x12 - x11) * (y21 - y11) + (y12 - y11) * (x11 - x21)) / divider;
    Point p(x21 + (x22 - x21) * v, y21 + (y22 - y21) * v);

    if (!equalF.IsAlmostBetween(p.x, x11, x12))
      return false;
    if (!equalF.IsAlmostBetween(p.x, x21, x22))
      return false;
    if (!equalF.IsAlmostBetween(p.y, y11, y12))
      return false;
    if (!equalF.IsAlmostBetween(p.y, y21, y22))
      return false;

    pt = p;
    return true;
  }

  static bool IsIntersect(Point const & p1, Point const & p2, Point const & p3, Point const & p4,
                          Point & pt)
  {
    return IsIntersect(p1.x, p1.y, p2.x, p2.y, p3.x, p3.y, p4.x, p4.y,
                       typename TraitsT::EqualType(), pt);
  }

  /// Taken from Computational Geometry in C and modified
  template <typename TEqualF>
  bool Contains(Point const & pt, TEqualF equalF) const
  {
    if (!m_rect.IsPointInside(pt))
      return false;

    int rCross = 0; /* number of right edge/ray crossings */
    int lCross = 0; /* number of left edge/ray crossings */

    size_t const numPoints = m_points.size();

    using BigCoordT = typename TraitsT::BigType;
    using BigPoint = ::m2::Point<BigCoordT>;

    BigPoint prev = BigPoint(m_points[numPoints - 1]) - BigPoint(pt);
    for (size_t i = 0; i < numPoints; ++i)
    {
      if (equalF.EqualPoints(m_points[i], pt))
        return true;

      BigPoint const curr = BigPoint(m_points[i]) - BigPoint(pt);

      bool const rCheck = ((curr.y > 0) != (prev.y > 0));
      bool const lCheck = ((curr.y < 0) != (prev.y < 0));

      if (rCheck || lCheck)
      {
        ASSERT_NOT_EQUAL(curr.y, prev.y, ());

        BigCoordT const delta = prev.y - curr.y;
        BigCoordT const cp = CrossProduct(curr, prev);

        // Squared precision is needed here because of comparison between cross product of two
        // std::vectors and zero. It's impossible to compare them relatively, so they're compared
        // absolutely, and, as cross product is proportional to product of lengths of both
        // operands precision must be squared too.
        if (!equalF.EqualZeroSquarePrecision(cp))
        {
          bool const PrevGreaterCurr = delta > 0.0;

          if (rCheck && ((cp > 0) == PrevGreaterCurr))
            ++rCross;
          if (lCheck && ((cp > 0) != PrevGreaterCurr))
            ++lCross;
        }
      }

      prev = curr;
    }

    /* q on the edge if left and right cross are not the same parity. */
    if ((rCross & 1) != (lCross & 1))
      return true;  // on the edge

    /* q inside if an odd number of crossings. */
    if (rCross & 1)
      return true;  // inside
    else
      return false;  // outside
  }

  bool Contains(Point const & pt) const { return Contains(pt, typename TraitsT::EqualType()); }

  /// Finds point of intersection with the section.
  bool FindIntersection(Point const & point1, Point const & point2, Point & result) const
  {
    if (m_points.empty())
      return false;
    Point const * prev = &m_points.back();
    for (Point const & curr : m_points)
    {
      if (IsIntersect(point1, point2, *prev, curr, result))
        return true;
      prev = &curr;
    }
    return false;
  }

  /// Slow check that point lies at the border.
  template <typename TEqualF>
  bool AtBorder(Point const & pt, double const delta, TEqualF equalF) const
  {
    if (!m_rect.IsPointInside(pt))
      return false;

    const double squareDelta = delta * delta;
    size_t const numPoints = m_points.size();

    Point prev = m_points[numPoints - 1];
    DistanceToLineSquare<Point> distance;
    for (size_t i = 0; i < numPoints; ++i)
    {
      Point const curr = m_points[i];

      // Borders often have same points with ways
      if (equalF.EqualPoints(m_points[i], pt))
        return true;

      distance.SetBounds(prev, curr);
      if (distance(pt) < squareDelta)
        return true;

      prev = curr;
    }

    return false;  // Point lies outside the border.
  }

  bool AtBorder(Point const & pt, double const delta) const
  {
    return AtBorder(pt, delta, typename TraitsT::EqualType());
  }

private:
  template <typename Archive, typename Pt>
  friend Archive & operator<<(Archive & ar, Region<Pt> const & region);

  template <typename Archive, typename Pt>
  friend Archive & operator>>(Archive & ar, Region<Pt> & region);

  template <typename T>
  friend std::string DebugPrint(Region<T> const &);

  void CalcLimitRect()
  {
    m_rect.MakeEmpty();
    for (size_t i = 0; i < m_points.size(); ++i)
      m_rect.Add(m_points[i]);
  }

  ContainerT m_points;
  m2::Rect<CoordT> m_rect;
};

template <typename Point>
void swap(Region<Point> & r1, Region<Point> & r2)
{
  r1.Swap(r2);
}

template <typename Archive, typename Point>
Archive & operator>>(Archive & ar, Region<Point> & region)
{
  ar >> region.m_rect;
  ar >> region.m_points;
  return ar;
}

template <typename Archive, typename Point>
Archive & operator<<(Archive & ar, Region<Point> const & region)
{
  ar << region.m_rect;
  ar << region.m_points;
  return ar;
}

template <typename Point>
std::string DebugPrint(Region<Point> const & r)
{
  return (DebugPrint(r.m_rect) + ::DebugPrint(r.m_points));
}

template <typename Point>
bool RegionsContain(std::vector<Region<Point>> const & regions, Point const & point)
{
  for (auto const & region : regions)
  {
    if (region.Contains(point))
      return true;
  }

  return false;
}

using RegionD = Region<m2::PointD>;
using RegionI = Region<m2::PointI>;
using RegionU = Region<m2::PointU>;
}  // namespace m2
