#include "geometry/algorithm.hpp"
#include "geometry/triangle2d.hpp"

#include "base/logging.hpp"

namespace m2
{
// CalculatePolyLineCenter -------------------------------------------------------------------------
void CalculatePolyLineCenter::operator()(m2::PointD const & pt)
{
  m_length += (m_poly.empty() ? 0.0 : m_poly.back().m_p.Length(pt));
  m_poly.emplace_back(pt, m_length);
}

PointD CalculatePolyLineCenter::GetResult() const
{
  using TIter = std::vector<Value>::const_iterator;

  double const l = m_length / 2.0;

  TIter e = lower_bound(m_poly.begin(), m_poly.end(), Value(m2::PointD(0, 0), l));
  if (e == m_poly.begin())
  {
    /// @todo It's very strange, but we have linear objects with zero length.
    LOG(LWARNING, ("Zero length linear object"));
    return e->m_p;
  }

  TIter b = e - 1;

  double const f = (l - b->m_len) / (e->m_len - b->m_len);

  // For safety reasons (floating point calculations) do comparison instead of ASSERT.
  if (0.0 <= f && f <= 1.0)
    return (b->m_p * (1 - f) + e->m_p * f);
  else
    return ((b->m_p + e->m_p) / 2.0);
}

// CalculatePointOnSurface -------------------------------------------------------------------------
CalculatePointOnSurface::CalculatePointOnSurface(m2::RectD const & rect)
  : m_rectCenter(rect.Center())
  , m_center(m_rectCenter)
  , m_squareDistanceToApproximate(std::numeric_limits<double>::max())
{}

void CalculatePointOnSurface::operator()(PointD const & p1, PointD const & p2, PointD const & p3)
{
  if (m_squareDistanceToApproximate == 0.0)
    return;
  if (m2::IsPointInsideTriangle(m_rectCenter, p1, p2, p3))
  {
    m_center = m_rectCenter;
    m_squareDistanceToApproximate = 0.0;
    return;
  }
  PointD triangleCenter(p1);
  triangleCenter += p2;
  triangleCenter += p3;
  triangleCenter = triangleCenter / 3.0;

  double triangleDistance = m_rectCenter.SquaredLength(triangleCenter);
  if (triangleDistance <= m_squareDistanceToApproximate)
  {
    m_center = triangleCenter;
    m_squareDistanceToApproximate = triangleDistance;
  }
}

/// @todo Make distances.size() == points.size() and unify with:
/// - CalculatePolyLineCenter
/// - Track::GetPoint(distance)
PointD InterpolatePointAtDistance(std::vector<double> const & distances, std::vector<PointD> const & points,
                                  double targetDistance)
{
  ASSERT_EQUAL(distances.size() + 1, points.size(), ());

  if (targetDistance <= 0)
    return points.front();

  if (targetDistance >= distances.back())
    return points.back();

  auto const it = std::upper_bound(distances.begin(), distances.end(), targetDistance);
  size_t const idx = std::distance(distances.begin(), it);

  // distances[idx-1] <= targetDistance < distances[idx], points[idx] .. points[idx+1]
  double const prevDist = idx > 0 ? distances[idx - 1] : 0.0;
  double const segLen = distances[idx] - prevDist;

  // segLen == 0 is unreachable: upper_bound skips duplicates to the first strictly-greater element.
  ASSERT(segLen > 0, (targetDistance, prevDist, distances[idx]));
  double const f = (targetDistance - prevDist) / segLen;
  return points[idx] + (points[idx + 1] - points[idx]) * f;
}
}  // namespace m2
