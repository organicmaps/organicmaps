#pragma once
#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

#include "std/map.hpp"
#include "std/shared_ptr.hpp"

namespace traffic
{
class TrafficCache
{
public:
  TrafficCache() : m_trafficColoring() {}
  virtual ~TrafficCache() = default;

  virtual shared_ptr<traffic::TrafficInfo::Coloring> GetTrafficInfo(MwmSet::MwmId const & mwmId) const;

protected:
  void Set(MwmSet::MwmId const & mwmId, TrafficInfo::Coloring && mwmIdAndColoring);
  void Remove(MwmSet::MwmId const & mwmId);
  void Clear();

private:
  map<MwmSet::MwmId, shared_ptr<traffic::TrafficInfo::Coloring>> m_trafficColoring;
};
}  // namespace traffic
