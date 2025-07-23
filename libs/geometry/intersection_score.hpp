#pragma once

#include "geometry/any_rect2d.hpp"
#include "geometry/point2d.hpp"

#include "base/exception.hpp"
#include "base/math.hpp"

#include <vector>

#include <boost/geometry/geometries/adapted/boost_tuple.hpp>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/geometry/geometries/polygon.hpp>
#include "std/boost_geometry.hpp"

namespace geometry
{
double constexpr kPenaltyScore = -1.0;
DECLARE_EXCEPTION(NotAPolygonException, RootException);

namespace impl
{
using PointXY = boost::geometry::model::d2::point_xy<double>;
using Polygon = boost::geometry::model::polygon<PointXY>;
using MultiPolygon = boost::geometry::model::multi_polygon<Polygon>;
}  // namespace impl

template <typename Container>
impl::Polygon PointsToPolygon(Container const & points);
template <typename Container>
impl::MultiPolygon TrianglesToPolygon(Container const & points);

// The return value is a real number from [-1.0, 1.0].
// * Returns positive value when the geometries intersect (returns intersection area divided by union area).
//   In particular, returns 1.0 when the geometries are equal.
// * Returns zero when the geometries do not intersect.
// * Returns kPenaltyScore as penalty. It is possible when any of the geometries is empty or invalid.
/// |lhs| and |rhs| are any areal boost::geometry types.
template <typename LGeometry, typename RGeometry>
double GetIntersectionScore(LGeometry const & lhs, RGeometry const & rhs)
{
  if (!boost::geometry::is_valid(lhs) || !boost::geometry::is_valid(rhs) || boost::geometry::is_empty(lhs) ||
      boost::geometry::is_empty(rhs))
  {
    return kPenaltyScore;
  }

  auto const lhsArea = boost::geometry::area(lhs);
  auto const rhsArea = boost::geometry::area(rhs);
  impl::MultiPolygon result;
  boost::geometry::intersection(lhs, rhs, result);
  auto const intersectionArea = boost::geometry::area(result);
  auto const unionArea = lhsArea + rhsArea - intersectionArea;

  auto const score = intersectionArea / unionArea;

  return score;
}

/// Throws NotAPolygonException exception.
/// For detailed info see comment for
/// double GetIntersectionScore(LPolygon const & lhs, RPolygon const & rhs).
/// |lhs| and |rhs| are any standard container of m2::Point with random access iterator.
/// |toPolygonConverter| is a method which converts |lhs| and |rhs| to boost::geometry areal type.
template <typename Container, typename Converter>
double GetIntersectionScore(Container const & lhs, Container const & rhs, Converter const & toPolygonConverter)
{
  auto const lhsPolygon = toPolygonConverter(lhs);
  if (boost::geometry::is_empty(lhsPolygon))
    return kPenaltyScore;

  auto const rhsPolygon = toPolygonConverter(rhs);
  if (boost::geometry::is_empty(rhsPolygon))
    return kPenaltyScore;

  return GetIntersectionScore(lhsPolygon, rhsPolygon);
}

/// Throws NotAPolygonException exception.
/// For detailed info see comment for
/// double GetIntersectionScore(LPolygon const & lhs, RPolygon const & rhs).
/// |lhs| and |rhs| are any standard containers of m2::Point with random access iterator.
template <typename Container>
double GetIntersectionScoreForPoints(Container const & lhs, Container const & rhs)
{
  return GetIntersectionScore<Container>(lhs, rhs, PointsToPolygon<Container>);
}

/// Throws NotAPolygonException exception.
/// For detailed info see comment for
/// double GetIntersectionScore(LPolygon const & lhs, RPolygon const & rhs).
/// |lhs| and |rhs| are any standard containers of m2::Point with random access iterator.
template <typename Container>
double GetIntersectionScoreForTriangulated(Container const & lhs, Container const & rhs)
{
  return GetIntersectionScore<Container>(lhs, rhs, TrianglesToPolygon<Container>);
}

/// |points| is any standard container of m2::Point with random access iterator.
template <typename Container>
impl::Polygon PointsToPolygon(Container const & points)
{
  impl::Polygon polygon;
  for (auto const & point : points)
    polygon.outer().push_back(impl::PointXY(point.x, point.y));
  boost::geometry::correct(polygon);
  if (!boost::geometry::is_valid(polygon))
    MYTHROW(geometry::NotAPolygonException, ("The points is not valid polygon"));

  return polygon;
}

/// |points| is any standard container of m2::Point with random access iterator.
template <typename Container>
impl::MultiPolygon TrianglesToPolygon(Container const & points)
{
  size_t const kTriangleSize = 3;
  if (points.size() % kTriangleSize != 0)
    MYTHROW(geometry::NotAPolygonException, ("Count of points must be multiple of", kTriangleSize));

  std::vector<impl::MultiPolygon> polygons;
  for (size_t i = 0; i < points.size(); i += kTriangleSize)
  {
    impl::MultiPolygon polygon;
    polygon.resize(1);
    auto & p = polygon[0];
    auto & outer = p.outer();
    for (size_t j = i; j < i + kTriangleSize; ++j)
      outer.push_back(impl::PointXY(points[j].x, points[j].y));
    boost::geometry::correct(p);
    if (!boost::geometry::is_valid(polygon))
      MYTHROW(geometry::NotAPolygonException, ("The triangle is not valid"));
    polygons.push_back(polygon);
  }

  if (polygons.empty())
    return {};

  auto & result = polygons[0];
  for (size_t i = 1; i < polygons.size(); ++i)
  {
    impl::MultiPolygon u;
    boost::geometry::union_(result, polygons[i], u);
    u.swap(result);
  }
  return result;
}
}  // namespace geometry
