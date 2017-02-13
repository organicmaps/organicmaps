#include "openlr/openlr_model.hpp"

#include "geometry/mercator.hpp"

namespace openlr
{
// LinearSegment -----------------------------------------------------------------------------------
std::vector<m2::PointD> LinearSegment::GetMercatorPoints() const
{
  std::vector<m2::PointD> points;
  for (auto const & point : m_locationReference.m_points)
    points.push_back(MercatorBounds::FromLatLon(point.m_latLon));
  return points;
}
}  // namespace openlr
