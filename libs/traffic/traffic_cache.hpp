#pragma once

#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace traffic
{
using AllMwmTrafficInfo = std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring const>>;

class TrafficCache
{
public:
  TrafficCache() = default;
  virtual ~TrafficCache() = default;

  virtual void CopyTraffic(AllMwmTrafficInfo & trafficColoring) const;

protected:
  void Set(MwmSet::MwmId const & mwmId, std::shared_ptr<TrafficInfo::Coloring const> coloring);
  void Remove(MwmSet::MwmId const & mwmId);
  void Clear();

private:
  mutable std::mutex m_mutex;
  AllMwmTrafficInfo m_trafficColoring;
};
}  // namespace traffic
