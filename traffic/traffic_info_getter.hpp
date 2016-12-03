#pragma once
#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

#include "std/map.hpp"
#include "std/shared_ptr.hpp"

namespace traffic
{
class TrafficInfoGetter
{
public:
  virtual ~TrafficInfoGetter() = default;

  virtual shared_ptr<traffic::TrafficInfo> GetTrafficInfo(MwmSet::MwmId const & mwmId) const = 0;
};

class TrafficCache final
{
public:
  void Set(traffic::TrafficInfo && info);
  void Remove(MwmSet::MwmId const & mwmId);
  shared_ptr<traffic::TrafficInfo> Get(MwmSet::MwmId const & mwmId) const;
  void Clear();

private:
  map<MwmSet::MwmId, shared_ptr<traffic::TrafficInfo>> m_trafficInfo;
};
}  // namespace traffic
