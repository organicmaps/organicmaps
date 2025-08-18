#include "generator/collector_mini_roundabout.hpp"

#include "base/assert.hpp"

#include <algorithm>
#include <iterator>

namespace generator
{
using namespace feature;

MiniRoundaboutCollector::MiniRoundaboutCollector(std::string const & filename, IDRInterfacePtr cache)
  : generator::CollectorInterface(filename)
  , m_cache(std::move(cache))
{}

std::shared_ptr<generator::CollectorInterface> MiniRoundaboutCollector::Clone(IDRInterfacePtr const & cache) const
{
  return std::make_shared<MiniRoundaboutCollector>(GetFilename(), cache);
}

void MiniRoundaboutCollector::Collect(OsmElement const & element)
{
  if (element.IsNode() && element.HasTag("highway", "mini_roundabout"))
  {
    CHECK(m_miniRoundabouts.emplace(element.m_id, element).second, ());
  }
  else if (element.IsRelation())
  {
    // Skip mini_roundabouts with role="via" in restrictions
    for (auto const & member : element.m_members)
    {
      if (member.m_type == OsmElement::EntityType::Node && member.m_role == "via")
      {
        m_miniRoundaboutsExceptions.insert(member.m_ref);
        break;
      }
    }
  }
}

void MiniRoundaboutCollector::CollectFeature(FeatureBuilder const & feature, OsmElement const & element)
{
  if (MiniRoundaboutInfo::IsProcessRoad(feature))
    m_roads.AddWay(element);
}

void MiniRoundaboutCollector::Save()
{
  /// @todo We assign only car roads here into MiniRoundaboutInfo.m_ways.
  /// Should also collect other highways (like path or pedestrian) in very general case.
  /// https://www.openstreetmap.org/way/220672898
  m_roads.ForEachWay([this](uint64_t id, std::vector<uint64_t> const & nodes)
  {
    for (uint64_t node : nodes)
    {
      auto it = m_miniRoundabouts.find(node);
      if (it != m_miniRoundabouts.end())
        it->second.m_ways.push_back(id);
    }
  }, m_cache);

  FileWriter writer(GetFilename());
  ForEachMiniRoundabout([&writer](MiniRoundaboutInfo & miniRoundabout)
  {
    if (miniRoundabout.Normalize())
      WriteMiniRoundabout(writer, miniRoundabout);
  });
}

void MiniRoundaboutCollector::MergeInto(MiniRoundaboutCollector & collector) const
{
  m_roads.MergeInto(collector.m_roads);
  collector.m_miniRoundabouts.insert(m_miniRoundabouts.begin(), m_miniRoundabouts.end());
  collector.m_miniRoundaboutsExceptions.insert(m_miniRoundaboutsExceptions.begin(), m_miniRoundaboutsExceptions.end());
}

}  // namespace generator
