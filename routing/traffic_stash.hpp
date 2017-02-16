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
    Guard(TrafficStash & stash) : m_stash(stash) {}

    ~Guard() { m_stash.Clear(); }

  private:
    TrafficStash & m_stash;
  };

  TrafficStash(traffic::TrafficCache const & source) : m_source(source) {}

  traffic::TrafficInfo::Coloring const * Get(NumMwmId numMwmId) const
  {
    auto it = m_mwmToTraffic.find(numMwmId);
    if (it == m_mwmToTraffic.cend())
      return nullptr;

    return it->second.get();
  }

  bool Has(NumMwmId numMwmId) const
  {
    return m_mwmToTraffic.find(numMwmId) != m_mwmToTraffic.cend();
  }

  void Load(MwmSet::MwmId const & mwmId, NumMwmId numMwmId)
  {
    auto traffic = m_source.GetTrafficInfo(mwmId);
    if (traffic)
      m_mwmToTraffic[numMwmId] = traffic;
  }

private:
  void Clear() { m_mwmToTraffic.clear(); }

  traffic::TrafficCache const & m_source;
  std::unordered_map<NumMwmId, shared_ptr<traffic::TrafficInfo::Coloring>> m_mwmToTraffic;
};
}  // namespace routing
