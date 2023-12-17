#include "openlr/openlr_model.hpp"

#include "geometry/mercator.hpp"

#include "base/assert.hpp"

using namespace std;

namespace openlr
{
// LinearSegment -----------------------------------------------------------------------------------
vector<m2::PointD> LinearSegment::GetMercatorPoints() const
{
  vector<m2::PointD> points;
  points.reserve(m_locationReference.m_points.size());
  for (auto const & point : m_locationReference.m_points)
    points.push_back(mercator::FromLatLon(point.m_latLon));
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

string DebugPrint(LinearSegmentSource source)
{
  switch (source)
  {
  case LinearSegmentSource::NotValid: return "NotValid";
  case LinearSegmentSource::FromLocationReferenceTag: return "FromLocationReferenceTag";
  case LinearSegmentSource::FromCoordinatesTag: return "FromCoordinatesTag";
  }
  UNREACHABLE();
}
}  // namespace openlr
