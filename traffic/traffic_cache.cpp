#include "traffic/traffic_cache.hpp"

namespace traffic
{
using namespace std;

void TrafficCache::Set(MwmSet::MwmId const & mwmId, shared_ptr<TrafficInfo::Coloring> coloring)
{
  m_trafficColoring[mwmId] = coloring;
}

void TrafficCache::Remove(MwmSet::MwmId const & mwmId) { m_trafficColoring.erase(mwmId); }

shared_ptr<TrafficInfo::Coloring> TrafficCache::GetTrafficInfo(MwmSet::MwmId const & mwmId) const
{
  auto it = m_trafficColoring.find(mwmId);

  if (it == m_trafficColoring.cend())
    return shared_ptr<TrafficInfo::Coloring>();
  return it->second;
}

void TrafficCache::CopyTraffic(
    map<MwmSet::MwmId, shared_ptr<traffic::TrafficInfo::Coloring>> & trafficColoring) const
{
  trafficColoring = m_trafficColoring;
}

void TrafficCache::Clear() { m_trafficColoring.clear(); }
}  // namespace traffic
