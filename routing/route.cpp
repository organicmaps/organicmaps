#include "route.hpp"

#include "../indexer/mercator.hpp"

#include "../geometry/distance_on_sphere.hpp"

namespace routing
{

double GetDistanceOnEarth(m2::PointD const & p1, m2::PointD const & p2)
{
  return ms::DistanceOnEarth(MercatorBounds::YToLat(p1.y),
                             MercatorBounds::XToLon(p1.x),
                             MercatorBounds::YToLat(p2.y),
                             MercatorBounds::XToLon(p2.x));
};


string DebugPrint(Route const & r)
{
  return DebugPrint(r.m_poly);
}

Route::Route(string const & router, vector<m2::PointD> const & points, string const & name)
  : m_router(router), m_poly(points), m_name(name), m_currentSegment(0)
{
  UpdateSegInfo();
}

double Route::GetDistance() const
{
  double distance = 0.0;
  size_t const n = m_poly.GetSize() - 1;
  for (size_t i = 0; i < n; ++i)
    distance += m_segDistance[i];

  return distance;
}

double Route::GetDistanceToTarget(m2::PointD const & currPos, double errorRadius, double predictDistance) const
{
  if (m_poly.GetSize() == m_currentSegment)
    return -1;

  pair<m2::PointD, size_t> res;
  if (predictDistance >= 0)
  {
    res = GetClosestProjection(currPos, errorRadius, [&] (m2::PointD const & pt, size_t i)
    {
      return fabs(GetDistanceOnPolyline(m_currentSegment, m_currentPoint, i - 1, pt) - predictDistance);
    });
  }
  else
  {
    res = GetClosestProjection(currPos, errorRadius, [&] (m2::PointD const & pt, size_t i)
    {
      return GetDistanceOnEarth(pt, currPos);
    });
  }

  if (res.second == -1)
    return -1;

  m_currentPoint = res.first;
  m_currentSegment = res.second;

  return GetDistanceOnPolyline(m_currentSegment, m_currentPoint, m_poly.GetSize() - 1, m_poly.Back());
}

double Route::GetDistanceOnPolyline(size_t s1, m2::PointD const & p1, size_t s2, m2::PointD const & p2) const
{
  size_t const n = m_poly.GetSize();

  ASSERT_LESS_OR_EQUAL(s1, s2, ());
  ASSERT_LESS(s1, n, ());
  ASSERT_LESS(s2, n, ());

  if (s1 == s2)
    return GetDistanceOnEarth(p1, p2);

  double dist = GetDistanceOnEarth(p1, m_poly.GetPoint(s1 + 1));
  for (size_t i = s1 + 1; i < s2; ++i)
    dist += m_segDistance[i];
  dist += GetDistanceOnEarth(p2, m_poly.GetPoint(s2));

  return dist;
}


void Route::UpdateSegInfo()
{
  size_t const n = m_poly.GetSize();
  ASSERT_GREATER(n, 1, ());

  m_segDistance.resize(n - 1);
  m_segProj.resize(n - 1);

  for (size_t i = 1; i < n; ++i)
  {
    m2::PointD const & p1 = m_poly.GetPoint(i - 1);
    m2::PointD const & p2 = m_poly.GetPoint(i);

    m_segDistance[i - 1] = ms::DistanceOnEarth(MercatorBounds::YToLat(p1.y),
                                               MercatorBounds::XToLon(p1.x),
                                               MercatorBounds::YToLat(p2.y),
                                               MercatorBounds::XToLon(p2.x));

    m_segProj[i - 1].SetBounds(p1, p2);
  }

  m_currentSegment = 0;
  m_currentPoint = m_poly.Front();
}

template <class DistanceF>
pair<m2::PointD, size_t> Route::GetClosestProjection(m2::PointD const & currPos, double errorRadius, DistanceF const & distFn) const
{
  double const errorRadius2 = errorRadius * errorRadius;

  size_t minSeg = -1;
  m2::PointD minPt;
  double minDist = numeric_limits<double>::max();

  for (size_t i = m_currentSegment + 1; i < m_poly.GetSize(); ++i)
  {
    m2::PointD const pt = m_segProj[i - 1](currPos);
    double const d = pt.SquareLength(currPos);

    if (d > errorRadius2)
      continue;

    double const dp = distFn(pt, i);

    if (dp < minDist)
    {
      minSeg = i - 1;
      minPt = pt;
      minDist = dp;
    }
  }

  return make_pair(minPt, minSeg);
}

} // namespace routing
