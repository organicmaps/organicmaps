#include "search/projection_on_street.hpp"

#include "geometry/mercator.hpp"

#include "geometry/robust_orientation.hpp"

#include <limits>

namespace search
{
// ProjectionOnStreet ------------------------------------------------------------------------------
ProjectionOnStreet::ProjectionOnStreet() : m_proj(0, 0), m_distMeters(0), m_segIndex(0), m_projSign(false) {}

// ProjectionOnStreetCalculator --------------------------------------------------------------------
ProjectionOnStreetCalculator::ProjectionOnStreetCalculator(std::vector<m2::PointD> const & points)
{
  size_t const count = points.size();
  if (count < 2)
    return;

  m_segments.reserve(count - 1);
  for (size_t i = 1; i < count; ++i)
    m_segments.emplace_back(points[i - 1], points[i]);
}

bool ProjectionOnStreetCalculator::GetProjection(m2::PointD const & point, ProjectionOnStreet & proj) const
{
  size_t const kInvalidIndex = m_segments.size();
  proj.m_segIndex = kInvalidIndex;
  proj.m_distMeters = std::numeric_limits<double>::max();

  for (size_t index = 0; index < m_segments.size(); ++index)
  {
    m2::PointD const ptProj = m_segments[index].ClosestPointTo(point);
    double const distMeters = mercator::DistanceOnEarth(point, ptProj);
    if (distMeters < proj.m_distMeters)
    {
      proj.m_proj = ptProj;
      proj.m_distMeters = distMeters;
      proj.m_segIndex = index;
      proj.m_projSign = m2::robust::OrientedS(m_segments[index].GetP0(), m_segments[index].GetP1(), point) <= 0.0;
    }
  }

  return (proj.m_segIndex < kInvalidIndex);
}
}  // namespace search
