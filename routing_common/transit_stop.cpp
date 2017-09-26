#include "routing_common/transit_stop.hpp"

#include <sstream>

namespace routing
{
namespace transit
{
Stop::Stop(StopId id, FeatureId featureId, std::vector<LineId> const & lineIds,
           m2::PointD const & point)
  : m_id(id), m_featureId(featureId), m_lineIds(lineIds), m_point(point)
{
}

bool Stop::IsEqualForTesting(Stop const & stop) const
{
  double constexpr kPointsEqualEpsilon = 1e-6;
  return m_id == stop.m_id && m_featureId == stop.m_featureId && m_lineIds == stop.m_lineIds &&
         my::AlmostEqualAbs(m_point, stop.m_point, kPointsEqualEpsilon);
}

std::string DebugPrint(Stop const & stop)
{
  std::stringstream out;
  out << "Stop [ m_id = " << stop.m_id
      << ", m_featureId = " << stop.m_featureId
      << ", m_lineIds = " << ::DebugPrint(stop.m_lineIds)
      << ", m_point = " << DebugPrint(stop.m_point)
      << " ]" << endl;
  return out.str();
}
}  // namespace transit
}  // namespace routing
