#pragma once

#include "routing/routing_mapping.hpp"
#include "routing/segment.hpp"
#include "routing/transition_points.hpp"

#include "routing_common/num_mwm_id.hpp"

#include "geometry/latlon.hpp"

#include "platform/country_file.hpp"

#include <map>
#include <memory>
#include <vector>

namespace routing
{
struct BorderCross;
class CrossMwmRoadGraph;

using TransitionPoints = buffer_vector<m2::PointD, 1>;

class CrossMwmOsrmGraph final
{
public:
  CrossMwmOsrmGraph(std::shared_ptr<NumMwmIds> numMwmIds, RoutingIndexManager & indexManager);
  ~CrossMwmOsrmGraph();

  bool IsTransition(Segment const & s, bool isOutgoing);
  void GetEdgeList(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges);
  void Clear();
  TransitionPoints GetTransitionPoints(Segment const & s, bool isOutgoing);

  template <typename Fn>
  void ForEachTransition(NumMwmId numMwmId, bool isEnter, Fn && fn)
  {
    auto const & ts = GetSegmentMaps(numMwmId);
    for (auto const & kv : isEnter ? ts.m_ingoing : ts.m_outgoing)
      fn(kv.first);
  }

private:
  struct TransitionSegments
  {
    std::map<Segment, std::vector<ms::LatLon>> m_ingoing;
    std::map<Segment, std::vector<ms::LatLon>> m_outgoing;
  };

  /// \brief Inserts all ingoing and outgoing transition segments of mwm with |numMwmId|
  /// to |m_transitionCache|. It works for OSRM section.
  /// \returns Reference to TransitionSegments corresponded to |numMwmId|.
  TransitionSegments const & LoadSegmentMaps(NumMwmId numMwmId);

  /// \brief Fills |borderCrosses| of mwm with |mapping| according to |s|.
  /// \param mapping if |isOutgoing| == true |mapping| is mapping ingoing (from) border cross.
  /// If |isOutgoing| == false |mapping| is mapping outgoing (to) border cross.
  /// \note |s| and |isOutgoing| params have the same restrictions which described in
  /// GetEdgeList() method.
  void GetBorderCross(TRoutingMappingPtr const & mapping, Segment const & s, bool isOutgoing,
                      std::vector<BorderCross> & borderCrosses);

  TransitionSegments const & GetSegmentMaps(NumMwmId numMwmId);
  std::vector<ms::LatLon> const & GetIngoingTransitionPoints(Segment const & s);
  std::vector<ms::LatLon> const & GetOutgoingTransitionPoints(Segment const & s);

  template <typename Fn>
  void LoadWith(NumMwmId numMwmId, Fn && fn)
  {
    platform::CountryFile const & countryFile = m_numMwmIds->GetFile(numMwmId);
    TRoutingMappingPtr mapping = m_indexManager.GetMappingByName(countryFile.GetName());
    CHECK(mapping, ("No routing mapping file for countryFile:", countryFile));
    CHECK(mapping->IsValid(), ("Mwm:", countryFile, "was not loaded."));

    auto const it = m_mappingGuards.find(numMwmId);
    if (it == m_mappingGuards.cend())
    {
      m_mappingGuards[numMwmId] = make_unique<MappingGuard>(mapping);
      mapping->LoadCrossContext();
    }

    fn(mapping);
  }

  std::shared_ptr<NumMwmIds> m_numMwmIds;

  // OSRM based cross-mwm information.
  RoutingIndexManager & m_indexManager;
  /// \note According to the constructor CrossMwmRoadGraph is initialized with RoutingIndexManager
  /// &.
  /// But then it is copied by value to CrossMwmRoadGraph::RoutingIndexManager m_indexManager.
  /// It means that there're two copies of RoutingIndexManager in CrossMwmGraph.
  std::unique_ptr<CrossMwmRoadGraph> m_crossMwmGraph;

  std::map<NumMwmId, TransitionSegments> m_transitionCache;
  std::map<NumMwmId, std::unique_ptr<MappingGuard>> m_mappingGuards;
};
}  // namespace routing
