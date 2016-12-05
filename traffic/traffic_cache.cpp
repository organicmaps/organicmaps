#include "traffic/traffic_cache.hpp"

namespace traffic
{
void TrafficCache::Set(TrafficInfo && info)
{
  MwmSet::MwmId const mwmId = info.GetMwmId();
  m_trafficInfo[mwmId] = make_shared<TrafficInfo>(move(info));
}

void TrafficCache::Remove(MwmSet::MwmId const & mwmId) { m_trafficInfo.erase(mwmId); }

shared_ptr<TrafficInfo> TrafficCache::GetTrafficInfo(MwmSet::MwmId const & mwmId) const
{
  auto it = m_trafficInfo.find(mwmId);

  if (it == m_trafficInfo.cend())
    return shared_ptr<TrafficInfo>();
  return it->second;
}

void TrafficCache::Clear() { m_trafficInfo.clear(); }
}  // namespace traffic
