#pragma once

#include "routing/num_mwm_id.hpp"
#include "routing/segment.hpp"

#include "traffic/traffic_cache.hpp"
#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

#include "base/assert.hpp"

#include <memory>
#include <unordered_map>

namespace routing
{
class TrafficStash final
{
public:
  class Guard final
  {
  public:
    Guard(TrafficStash & stash) : m_stash(stash) { m_stash.CopyTraffic(); }
    ~Guard() { m_stash.Clear(); }

  private:
    TrafficStash & m_stash;
  };

  TrafficStash(traffic::TrafficCache const & source, std::shared_ptr<NumMwmIds> numMwmIds);

  traffic::SpeedGroup GetSpeedGroup(Segment const & segment) const;
  void SetColoring(NumMwmId numMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring> coloring);
  bool Has(NumMwmId numMwmId) const;

private:
  void CopyTraffic();

  void Clear() { m_mwmToTraffic.clear(); }

  traffic::TrafficCache const & m_source;
  shared_ptr<NumMwmIds> m_numMwmIds;
  std::unordered_map<NumMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> m_mwmToTraffic;
};
}  // namespace routing
