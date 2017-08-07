#pragma once

#include "routing/cross_mwm_index_graph.hpp"
#include "routing/cross_mwm_osrm_graph.hpp"
#include "routing/num_mwm_id.hpp"
#include "routing/segment.hpp"

#include "routing_common/vehicle_model.hpp"

#include "indexer/index.hpp"

#include "geometry/tree4d.hpp"

#include "base/math.hpp"

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace routing
{
/// \brief Getting information for cross mwm routing.
class CrossMwmGraph final
{
public:
  enum class MwmStatus
  {
    NotLoaded,
    CrossMwmSectionExists,
    NoCrossMwmSection,
  };

  CrossMwmGraph(std::shared_ptr<NumMwmIds> numMwmIds, shared_ptr<m4::Tree<NumMwmId>> numMwmTree,
                std::shared_ptr<VehicleModelFactoryInterface> vehicleModelFactory,
                CourntryRectFn const & countryRectFn, Index & index,
                RoutingIndexManager & indexManager);

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

  template <typename Fn>
  void ForEachTransition(NumMwmId numMwmId, bool isEnter, Fn && fn)
  {
    if (CrossMwmSectionExists(numMwmId))
      m_crossMwmIndexGraph.ForEachTransition(numMwmId, isEnter, std::forward<Fn>(fn));
    else
      m_crossMwmOsrmGraph.ForEachTransition(numMwmId, isEnter, std::forward<Fn>(fn));
  }

private:
  struct ClosestSegment
  {
    ClosestSegment();
    ClosestSegment(double minDistM, Segment const & bestSeg, bool exactMatchFound);
    void Update(double distM, Segment const & bestSeg);

    double m_bestDistM;
    Segment m_bestSeg;
    bool m_exactMatchFound;
  };

  /// \returns points of |s|. |s| should be a transition segment of mwm with an OSRM cross-mwm sections or
  /// with an index graph cross-mwm section.
  /// \param s is a transition segment of type |isOutgoing|.
  /// \note the result of the method is returned by value because the size of the vector is usually
  /// one or very small in rare cases in OSRM.
  TransitionPoints GetTransitionPoints(Segment const & s, bool isOutgoing);

  MwmStatus GetMwmStatus(NumMwmId numMwmId) const;
  bool CrossMwmSectionExists(NumMwmId numMwmId);

  /// \brief Fills |twins| with transition segments of feature |ft| of type |isOutgoing|.
  void GetTwinCandidates(FeatureType const & ft, bool isOutgoing,
                         std::vector<Segment> & twinCandidates);

  /// \brief Fills structure |twins| or for feature |ft| if |ft| contains transition segment(s).
  /// \param sMwmId mwm id of a segment which twins are looked for
  /// \param ft feature which could contain twin segments
  /// \param point point of a segment which twins are looked for
  /// \param minDistSegs is used to keep the best twin candidate
  /// \param twins is filled with twins if there're twins (one or more) that have a point which is
  ///        very near or equal to |point|.
  /// \note If the method finds twin segment with a point which is very close to |point| the twin segment is
  /// added to |twins| anyway. If there's no such segment in mwm it tries find the closet one and adds it
  /// to |minDistSegs|.
  void FindBestTwins(NumMwmId sMwmId, bool isOutgoing, FeatureType const & ft,
                     m2::PointD const & point, std::map<NumMwmId, ClosestSegment> & minDistSegs,
                     std::vector<Segment> & twins);

  /// \brief Fills |neighbors| with number mwm id of all loaded neighbors of |numMwmId| and
  /// sets |allNeighborsHaveCrossMwmSection| to true if all loaded neighbors have cross mwm section
  /// and to false otherwise.
  void GetAllLoadedNeighbors(NumMwmId numMwmId,
                             std::vector<NumMwmId> & neighbors,
                             bool & allNeighborsHaveCrossMwmSection);
  /// \brief Deserizlize transitions for mwm with |ids|.
  void DeserializeTransitions(std::vector<NumMwmId> const & mwmIds);

  Index & m_index;
  std::shared_ptr<NumMwmIds> m_numMwmIds;
  std::shared_ptr<m4::Tree<NumMwmId>> m_numMwmTree;
  std::shared_ptr<VehicleModelFactoryInterface> m_vehicleModelFactory;
  CourntryRectFn const & m_countryRectFn;
  CrossMwmIndexGraph m_crossMwmIndexGraph;
  CrossMwmOsrmGraph m_crossMwmOsrmGraph;
};

string DebugPrint(CrossMwmGraph::MwmStatus status);
}  // routing
