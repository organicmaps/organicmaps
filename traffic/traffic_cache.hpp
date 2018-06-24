#pragma once

#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

#include <map>
#include <memory>
#include <mutex>

namespace traffic
{
class TrafficCache
{
public:
  TrafficCache() : m_trafficColoring() {}
  virtual ~TrafficCache() = default;

  virtual void CopyTraffic(
      std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> & trafficColoring)
      const;

protected:
  void Set(MwmSet::MwmId const & mwmId, std::shared_ptr<TrafficInfo::Coloring> coloring);
  void Remove(MwmSet::MwmId const & mwmId);
  void Clear();

private:
  std::mutex m_mutex;
  std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> m_trafficColoring;
};
}  // namespace traffic
