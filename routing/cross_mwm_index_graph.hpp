#pragma once

#include "routing/num_mwm_id.hpp"
#include "routing/routing_mapping.hpp"
#include "routing/segment.hpp"

#include "geometry/latlon.hpp"

#include "platform/country_file.hpp"

#include "base/math.hpp"

#include <map>
#include <memory>
#include <vector>

namespace routing
{
struct BorderCross;
class CrossMwmGraph;

/// \brief Getting information for cross mwm routing.
class CrossMwmIndexGraph final
{
public:
  CrossMwmIndexGraph(std::shared_ptr<NumMwmIds> numMwmIds, RoutingIndexManager & indexManager);
  ~CrossMwmIndexGraph();

  /// \brief Transition segment is a segment which is crossed by mwm border. That means
  /// start and finsh of such segment have to lie in different mwms. If a segment is
  /// crossed by mwm border but its start and finish lie in the same mwm it's not
  /// a transition segment.
  /// For most cases there is only one transition segment for a transition feature.
  /// Transition segment at the picture below is marked as *===>*.
  /// Transition feature is a feature which contains one or more transition segments.
  /// Transition features at the picture below is marked as *===>*------>*.
  /// Every transition feature is duplicated in two neighbouring mwms.
  /// The duplicate is called "twin" at the picture below.
  /// For most cases a transition feature contains only one transition segment.
  /// -------MWM0---------------MWM1----------------MWM2-------------
  /// |                   |                 |                       |
  /// | F0 in MWM0   *===>*------>*    *===>*------>*  F2 of MWM1   |
  /// | Twin in MWM1 *===>*------>*    *===>*------>*  Twin in MWM2 |
  /// |                   |                 |                       |
  /// | F1 in MWM0      *===>*---->*      *===>*--->*  F3 of MWM1   |
  /// | Twin in MWM1    *===>*---->*      *===>*--->*  Twin in MWM2 |
  /// |                   |                 |                       |
  /// ---------------------------------------------------------------
  /// There are two kinds of transition segments:
  /// * enter transition segments are enters to their mwms;
  /// * exit transition segments are exits from their mwms;
  /// \returns true if |s| is an exit (|isOutgoing| == true) or an enter (|isOutgoing| == false)
  /// transition segment.
  bool IsTransition(Segment const & s, bool isOutgoing);

  /// \brief Fills |twins| with duplicates of |s| transition segment in neighbouring mwm.
  /// For most cases there is only one twin for |s|.
  /// If |isOutgoing| == true |s| should be an exit transition segment and
  /// the mehtod fills |twins| with appropriate enter transition segments.
  /// If |isOutgoing| == false |s| should be an enter transition segment and
  /// the method fills |twins| with appropriate exit transition segments.
  /// \note GetTwins(s, isOutgoing, ...) shall be called only if IsTransition(s, isOutgoing) returns true.
  /// \note GetTwins(s, isOutgoing, twins) fills |twins| only if mwm contained |twins| has been downloaded.
  /// If not, |twins| could be emply after a GetTwins(...) call.
  void GetTwins(Segment const & s, bool isOutgoing, std::vector<Segment> & twins);

  /// \brief Fills |edges| with edges outgoing from |s| (ingoing to |s|).
  /// If |isOutgoing| == true then |s| should be an enter transition segment.
  /// In that case |edges| is filled with all edges starting from |s| and ending at all reachable
  /// exit transition segments of the mwm of |s|.
  /// If |isOutgoing| == false then |s| should be an exit transition segment.
  /// In that case |edges| is filled with all edges starting from all reachable
  /// enter transition segments of the mwm of |s| and ending at |s|.
  /// Weight of each edge is equal to weight of the route form |s| to |SegmentEdge::m_target|
  /// if |isOutgoing| == true and from |SegmentEdge::m_target| to |s| otherwise.
  void GetEdgeList(Segment const & s, bool isOutgoing, std::vector<SegmentEdge> & edges);

  void Clear();

private:
  struct TransitionSegments
  {
    std::map<Segment, std::vector<ms::LatLon>> m_ingoing;
    std::map<Segment, std::vector<ms::LatLon>> m_outgoing;
  };

  void ResetCrossMwmGraph();

  /// \brief Inserts all ingoing and outgoing transition segments of mwm with |numMwmId|
  /// to |m_transitionCache|.
  void InsertWholeMwmTransitionSegments(NumMwmId numMwmId);

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

  template <class Fn>
  bool LoadWith(NumMwmId numMwmId, Fn && fn)
  {
    platform::CountryFile const & countryFile = m_numMwmIds->GetFile(numMwmId);
    TRoutingMappingPtr mapping = m_indexManager.GetMappingByName(countryFile.GetName());
    CHECK(mapping, ("No routing mapping file for countryFile:", countryFile));

    if (!mapping->IsValid())
      return false; // mwm was not loaded.

    auto const it = m_mappingGuards.find(numMwmId);
    if (it == m_mappingGuards.cend())
    {
      m_mappingGuards[numMwmId] = make_unique<MappingGuard>(mapping);
      mapping->LoadCrossContext();
    }

    fn(mapping);
    return true;
  }

  RoutingIndexManager & m_indexManager;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  /// \note According to the constructor CrossMwmGraph is initialized with RoutingIndexManager &.
  /// But then it is copied by value to CrossMwmGraph::RoutingIndexManager m_indexManager.
  /// It means that there're two copies of RoutingIndexManager in CrossMwmIndexGraph.
  std::unique_ptr<CrossMwmGraph> m_crossMwmGraph;

  std::map<NumMwmId, TransitionSegments> m_transitionCache;

  std::map<NumMwmId, std::unique_ptr<MappingGuard>> m_mappingGuards;
};
}  // routing
