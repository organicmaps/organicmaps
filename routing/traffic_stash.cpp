#include "routing/traffic_stash.hpp"

#include "base/checked_cast.hpp"

#include <map>

namespace routing
{
traffic::SpeedGroup TrafficStash::GetSpeedGroup(Segment const & segment) const
{
  auto itMwm = m_mwmToTraffic.find(segment.GetMwmId());
  if (itMwm == m_mwmToTraffic.cend())
    return traffic::SpeedGroup::Unknown;

  auto const & coloring = itMwm->second;
  auto const itSeg = coloring->find(traffic::TrafficInfo::RoadSegmentId(
      segment.GetFeatureId(), base::asserted_cast<uint16_t>(segment.GetSegmentIdx()),
      segment.IsForward() ? traffic::TrafficInfo::RoadSegmentId::kForwardDirection
                          : traffic::TrafficInfo::RoadSegmentId::kReverseDirection));

  if (itSeg == coloring->cend())
    return traffic::SpeedGroup::Unknown;

  return itSeg->second;
}

void TrafficStash::CopyTraffic()
{
  std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> copy;
  m_source.CopyTraffic(copy);
  for (auto const & kv : copy)
  {
    auto const numMwmId = m_numMwmIds->GetId(kv.first.GetInfo()->GetLocalFile().GetCountryFile());
    CHECK(kv.second, ());
    SetColoring(numMwmId, kv.second);
  }
}
}  // namespace routing
