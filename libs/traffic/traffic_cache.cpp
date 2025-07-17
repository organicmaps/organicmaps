#include "traffic/traffic_cache.hpp"

namespace traffic
{

void TrafficCache::Set(MwmSet::MwmId const & mwmId, std::shared_ptr<TrafficInfo::Coloring const> coloring)
{
  auto guard = std::lock_guard(m_mutex);
  m_trafficColoring[mwmId] = std::move(coloring);
}

void TrafficCache::Remove(MwmSet::MwmId const & mwmId)
{
  auto guard = std::lock_guard(m_mutex);
  m_trafficColoring.erase(mwmId);
}

void TrafficCache::CopyTraffic(AllMwmTrafficInfo & trafficColoring) const
{
  auto guard = std::lock_guard(m_mutex);
  trafficColoring = m_trafficColoring;
}

void TrafficCache::Clear()
{
  auto guard = std::lock_guard(m_mutex);
  m_trafficColoring.clear();
}
}  // namespace traffic
