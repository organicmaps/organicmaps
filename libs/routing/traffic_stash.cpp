#include "routing/traffic_stash.hpp"

#include "base/checked_cast.hpp"

#include <map>

namespace routing
{
using namespace std;

TrafficStash::TrafficStash(traffic::TrafficCache const & source, shared_ptr<NumMwmIds> numMwmIds)
  : m_source(source)
  , m_numMwmIds(std::move(numMwmIds))
{
  CHECK(m_numMwmIds, ());
}

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

void TrafficStash::SetColoring(NumMwmId numMwmId, shared_ptr<traffic::TrafficInfo::Coloring const> coloring)
{
  m_mwmToTraffic[numMwmId] = coloring;
}

bool TrafficStash::Has(NumMwmId numMwmId) const
{
  return m_mwmToTraffic.find(numMwmId) != m_mwmToTraffic.cend();
}

void TrafficStash::CopyTraffic()
{
  traffic::AllMwmTrafficInfo copy;
  m_source.CopyTraffic(copy);
  for (auto const & kv : copy)
  {
    auto const numMwmId = m_numMwmIds->GetId(kv.first.GetInfo()->GetLocalFile().GetCountryFile());
    CHECK(kv.second, ());
    SetColoring(numMwmId, kv.second);
  }
}
}  // namespace routing
