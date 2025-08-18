#pragma once

#include "routing/segment.hpp"

#include "traffic/traffic_cache.hpp"
#include "traffic/traffic_info.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "indexer/mwm_set.hpp"

#include "base/assert.hpp"

#include <memory>
#include <unordered_map>
#include <utility>

namespace routing
{
class TrafficStash final
{
public:
  class Guard final
  {
  public:
    explicit Guard(std::shared_ptr<TrafficStash> stash) : m_stash(std::move(stash))
    {
      if (m_stash)
        m_stash->CopyTraffic();
    }

    ~Guard()
    {
      if (m_stash)
        m_stash->Clear();
    }

  private:
    std::shared_ptr<TrafficStash> m_stash;
  };

  TrafficStash(traffic::TrafficCache const & source, std::shared_ptr<NumMwmIds> numMwmIds);

  traffic::SpeedGroup GetSpeedGroup(Segment const & segment) const;
  void SetColoring(NumMwmId numMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring const> coloring);
  bool Has(NumMwmId numMwmId) const;

private:
  void CopyTraffic();

  void Clear() { m_mwmToTraffic.clear(); }

  traffic::TrafficCache const & m_source;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::unordered_map<NumMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring const>> m_mwmToTraffic;
};
}  // namespace routing
