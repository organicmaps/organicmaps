#pragma once

#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

#include <map>
#include <memory>

namespace traffic
{
class TrafficCache
{
public:
  TrafficCache() : m_trafficColoring() {}
  virtual ~TrafficCache() = default;

  virtual shared_ptr<traffic::TrafficInfo::Coloring> GetTrafficInfo(
      MwmSet::MwmId const & mwmId) const;
  virtual void CopyTraffic(
      std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> & trafficColoring)
      const;

protected:
  void Set(MwmSet::MwmId const & mwmId, std::shared_ptr<TrafficInfo::Coloring> coloring);
  void Remove(MwmSet::MwmId const & mwmId);
  void Clear();

private:
  std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> m_trafficColoring;
};
}  // namespace traffic
