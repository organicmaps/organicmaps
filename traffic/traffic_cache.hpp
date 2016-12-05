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
  TrafficCache() : m_trafficInfo() {}
  virtual ~TrafficCache() = default;

  virtual shared_ptr<traffic::TrafficInfo> GetTrafficInfo(MwmSet::MwmId const & mwmId) const;

protected:
  void Set(traffic::TrafficInfo && info);
  void Remove(MwmSet::MwmId const & mwmId);
  void Clear();

private:
  map<MwmSet::MwmId, shared_ptr<traffic::TrafficInfo>> m_trafficInfo;
};
}  // namespace traffic
