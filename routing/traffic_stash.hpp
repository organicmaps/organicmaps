#pragma once

#include "routing/num_mwm_id.hpp"

#include "traffic/traffic_cache.hpp"
#include "traffic/traffic_info.hpp"

#include "indexer/mwm_set.hpp"

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

  TrafficStash(traffic::TrafficCache const & source, shared_ptr<NumMwmIds> numMwmIds)
    : m_source(source), m_numMwmIds(numMwmIds)
  {
  }

  traffic::TrafficInfo::Coloring const * Get(NumMwmId numMwmId) const
  {
    auto it = m_mwmToTraffic.find(numMwmId);
    if (it == m_mwmToTraffic.cend())
      return nullptr;

    return it->second.get();
  }

  void SetColoring(NumMwmId numMwmId, std::shared_ptr<traffic::TrafficInfo::Coloring> coloring)
  {
    m_mwmToTraffic[numMwmId] = coloring;
  }

  bool Has(NumMwmId numMwmId) const
  {
    return m_mwmToTraffic.find(numMwmId) != m_mwmToTraffic.cend();
  }

private:
  void CopyTraffic()
  {
    std::map<MwmSet::MwmId, std::shared_ptr<traffic::TrafficInfo::Coloring>> copy;
    m_source.CopyTraffic(copy);
    for (auto it : copy)
    {
      auto const numMwmId = m_numMwmIds->GetId(it.first.GetInfo()->GetLocalFile().GetCountryFile());
      SetColoring(numMwmId, it.second);
    }
  }

  void Clear() { m_mwmToTraffic.clear(); }

  traffic::TrafficCache const & m_source;
  shared_ptr<NumMwmIds> m_numMwmIds;
  std::unordered_map<NumMwmId, shared_ptr<traffic::TrafficInfo::Coloring>> m_mwmToTraffic;
};
}  // namespace routing
