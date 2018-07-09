#include "openlr/openlr_model.hpp"

#include "geometry/mercator.hpp"

using namespace std;

namespace openlr
{
// LinearSegment -----------------------------------------------------------------------------------
vector<m2::PointD> LinearSegment::GetMercatorPoints() const
{
  vector<m2::PointD> points;
  for (auto const & point : m_locationReference.m_points)
    points.push_back(MercatorBounds::FromLatLon(point.m_latLon));
  return points;
}

vector<LocationReferencePoint> const & LinearSegment::GetLRPs() const
{
  return m_locationReference.m_points;
}

vector<LocationReferencePoint> & LinearSegment::GetLRPs()
{
  return m_locationReference.m_points;
}
}  // namespace openlr
