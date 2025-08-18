#pragma once

#include "routing/cross_mwm_ids.hpp"
#include "routing/cross_mwm_index_graph.hpp"
#include "routing/regions_decl.hpp"
#include "routing/segment.hpp"
#include "routing/vehicle_mask.hpp"

#include "routing_common/num_mwm_id.hpp"
#include "routing_common/vehicle_model.hpp"

#include "geometry/tree4d.hpp"

#include "base/geo_object_id.hpp"

#include <memory>
#include <string>
#include <vector>

namespace routing
{
class MwmDataSource;

/// \brief Getting information for cross mwm routing.
class CrossMwmGraph final
{
public:
  enum class MwmStatus
  {
    NotLoaded,
    SectionExists,
    NoSection,
  };

  CrossMwmGraph(std::shared_ptr<NumMwmIds> numMwmIds, std::shared_ptr<m4::Tree<NumMwmId>> numMwmTree,
                VehicleType vehicleType, CountryRectFn const & countryRectFn, MwmDataSource & dataSource);

  /// \brief Transition segment is a segment which is crossed by mwm border. That means
  /// start and finish of such segment have to lie in different mwms. If a segment is
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
  /// the method fills |twins| with appropriate enter transition segments.
  /// If |isOutgoing| == false |s| should be an enter transition segment and
  /// the method fills |twins| with appropriate exit transition segments.
  /// \note GetTwins(s, isOutgoing, ...) shall be called only if IsTransition(s, isOutgoing) returns
  /// true.
  /// \note GetTwins(s, isOutgoing, twins) fills |twins| only if mwm contained |twins| has been
  /// downloaded.
  /// If not, |twins| could be emply after a GetTwins(...) call.
  void GetTwins(Segment const & s, bool isOutgoing, std::vector<Segment> & twins);

  using EdgeListT = CrossMwmIndexGraph<base::GeoObjectId>::EdgeListT;

  /// \brief Fills |edges| with edges outgoing from |s|.
  /// |s| should be an enter transition segment, |edges| is filled with all edges starting from |s|
  /// and ending at all reachable exit transition segments of the mwm of |s|.
  /// Weight of each edge is equal to weight of the route form |s| to |SegmentEdge::m_target|.
  /// Getting ingoing edges is not supported because we do not have enough information
  /// to calculate |segment| weight.
  void GetOutgoingEdgeList(Segment const & s, EdgeListT & edges);
  void GetIngoingEdgeList(Segment const & s, EdgeListT & edges);

  RouteWeight GetWeightSure(Segment const & from, Segment const & to);

  // void Clear();
  void Purge();

  template <class FnT>
  void ForEachTransition(NumMwmId numMwmId, bool isEnter, FnT && fn)
  {
    CHECK(CrossMwmSectionExists(numMwmId), ("Should be used in LeapsOnly mode only"));
    return m_crossMwmIndexGraph.ForEachTransition(numMwmId, isEnter, fn);
  }

  /// \brief Checks whether feature where |segment| is placed is a cross mwm connector.
  ///        If yes twin-segments are saved to |twins|.
  void GetTwinFeature(Segment const & segment, bool isOutgoing, std::vector<Segment> & twins);

private:
  MwmStatus GetMwmStatus(NumMwmId numMwmId, std::string const & sectionName) const;
  MwmStatus GetCrossMwmStatus(NumMwmId numMwmId) const;
  MwmStatus GetTransitCrossMwmStatus(NumMwmId numMwmId) const;
  bool CrossMwmSectionExists(NumMwmId numMwmId) const;
  bool TransitCrossMwmSectionExists(NumMwmId numMwmId) const;

  /// \brief Fills |neighbors| with number mwm id of all loaded neighbors of |numMwmId| and
  /// sets |allNeighborsHaveCrossMwmSection| to true if all loaded neighbors have cross mwm section
  /// and to false otherwise.
  bool GetAllLoadedNeighbors(NumMwmId numMwmId, std::vector<NumMwmId> & neighbors);
  /// \brief Deserizlize transitions for mwm with |ids|.
  void DeserializeTransitions(std::vector<NumMwmId> const & mwmIds);
  void DeserializeTransitTransitions(std::vector<NumMwmId> const & mwmIds);

  MwmDataSource & m_dataSource;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::shared_ptr<m4::Tree<NumMwmId>> m_numMwmTree;
  CountryRectFn const & m_countryRectFn;
  CrossMwmIndexGraph<base::GeoObjectId> m_crossMwmIndexGraph;
  CrossMwmIndexGraph<connector::TransitId> m_crossMwmTransitGraph;
};

std::string DebugPrint(CrossMwmGraph::MwmStatus status);
}  // namespace routing
