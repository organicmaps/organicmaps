#pragma once

#include "generator/collector_interface.hpp"
#include "generator/mini_roundabout_info.hpp"
#include "generator/way_nodes_mapper.hpp"

#include <unordered_map>
#include <unordered_set>

namespace generator
{

class MiniRoundaboutCollector : public generator::CollectorInterface
{
public:
  MiniRoundaboutCollector(std::string const & filename, IDRInterfacePtr cache);

  // CollectorInterface overrides:
  std::shared_ptr<CollectorInterface> Clone(IDRInterfacePtr const & = {}) const override;

  void Collect(OsmElement const & element) override;
  void CollectFeature(feature::FeatureBuilder const & feature, OsmElement const & element) override;

  IMPLEMENT_COLLECTOR_IFACE(MiniRoundaboutCollector);
  void MergeInto(MiniRoundaboutCollector & collector) const;

protected:
  void Save() override;

  template <typename Fn>
  void ForEachMiniRoundabout(Fn && toDo)
  {
    for (auto & p : m_miniRoundabouts)
      if (m_miniRoundaboutsExceptions.count(p.first) == 0)
        toDo(p.second);
  }

private:
  IDRInterfacePtr m_cache;

  WaysIDHolder m_roads;
  std::unordered_map<uint64_t, MiniRoundaboutInfo> m_miniRoundabouts;
  std::unordered_set<uint64_t> m_miniRoundaboutsExceptions;
};
}  // namespace generator
