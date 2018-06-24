#include "traffic/traffic_cache.hpp"

namespace traffic
{
using namespace std;

void TrafficCache::Set(MwmSet::MwmId const & mwmId, shared_ptr<TrafficInfo::Coloring> coloring)
{
  lock_guard<mutex> guard(mutex);
  m_trafficColoring[mwmId] = coloring;
}

void TrafficCache::Remove(MwmSet::MwmId const & mwmId)
{
  lock_guard<mutex> guard(mutex);
  m_trafficColoring.erase(mwmId);
}

void TrafficCache::CopyTraffic(
    map<MwmSet::MwmId, shared_ptr<traffic::TrafficInfo::Coloring>> & trafficColoring) const
{
  lock_guard<mutex> guard(mutex);
  trafficColoring = m_trafficColoring;
}

void TrafficCache::Clear()
{
  lock_guard<mutex> guard(mutex);
  m_trafficColoring.clear();
}
}  // namespace traffic
