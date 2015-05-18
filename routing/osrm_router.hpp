#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"
#include "routing/osrm_engine.hpp"
#include "routing/route.hpp"
#include "routing/router.hpp"
#include "routing/routing_mapping.h"

#include "indexer/index.hpp"
#include "base/mutex.hpp"

#include "std/atomic.hpp"
#include "std/function.hpp"
#include "std/numeric.hpp"
#include "std/queue.hpp"
#include "std/unordered_map.hpp"


namespace feature { class TypesHolder; }

struct RawRouteData;
struct PhantomNode;
struct PathData;
class FeatureType;

namespace routing
{
typedef function<string (m2::PointD const &)> CountryFileFnT;
typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;

/// All edges available for start route while routing
typedef vector<FeatureGraphNode> FeatureGraphNodeVecT;
/// Points vector to calculate several routes
typedef vector<FeatureGraphNode> MultiroutingTaskPointT;

typedef vector<RawRoutingResult> MultipleRoutingResultT;

/*! Manager for loading, cashing and building routing indexes.
 * Builds and shares special routing contexts.
*/
class RoutingIndexManager
{
  CountryFileFnT m_countryFn;

  unordered_map<string, RoutingMappingPtrT> m_mapping;

public:
  RoutingIndexManager(CountryFileFnT const & fn): m_countryFn(fn) {}

  RoutingMappingPtrT GetMappingByPoint(m2::PointD const & point, Index const * pIndex);

  RoutingMappingPtrT GetMappingByName(string const & fName, Index const * pIndex);

  template <class TFunctor>
  void ForEachMapping(TFunctor toDo)
  {
    for_each(m_mapping.begin(), m_mapping.end(), toDo);
  }

  void Clear()
  {
    m_mapping.clear();
  }
};

class OsrmRouter : public IRouter
{
public:
  typedef vector<size_t> NodeIdVectorT;
  typedef vector<double> GeomTurnCandidateT;

  OsrmRouter(Index const * index, CountryFileFnT const & fn, RoutingVisualizerFn routingVisualization = nullptr);

  virtual string GetName() const;

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, Route & route) override;

  virtual void ClearState();

  /*! Find single shortest path in a single MWM between 2 sets of edges
     * \param source: vector of source edges to make path
     * \param taget: vector of target edges to make path
     * \param facade: OSRM routing data facade to recover graph information
     * \param rawRoutingResult: routing result store
     * \return true when path exists, false otherwise.
     */
  static bool FindRouteFromCases(FeatureGraphNodeVecT const & source, FeatureGraphNodeVecT const & target, DataFacadeT & facade,
                       RawRoutingResult & rawRoutingResult);

protected:
  IRouter::ResultCode FindPhantomNodes(string const & fName, m2::PointD const & point,
                                       m2::PointD const & direction, FeatureGraphNodeVecT & res,
                                       size_t maxCount, RoutingMappingPtrT const & mapping);

  size_t FindNextMwmNode(OutgoingCrossNode const & startNode, RoutingMappingPtrT const & targetMapping);

  /*!
   * \brief Compute turn and time estimation structs for OSRM raw route.
   * \param routingResult OSRM routing result structure to annotate
   * \param mapping Feature mappings
   * \param points Unpacked point pathes
   * \param requestCancel flag to stop calculation
   * \param turnsDir output turns annotation storage
   * \param times output times annotation storage
   * \param turnsGeom output turns geometry
   * \return OSRM routing errors if any
   */
  ResultCode MakeTurnAnnotation(RawRoutingResult const & routingResult,
                                RoutingMappingPtrT const & mapping, vector<m2::PointD> & points,
                                Route::TurnsT & turnsDir, Route::TimesT & times,
                                turns::TurnsGeomT & turnsGeom);

private:
  typedef pair<size_t,string> MwmOutT;
  typedef set<MwmOutT> CheckedOutsT;

  struct RoutePathCross
  {
    string mwmName;
    FeatureGraphNode startNode;
    FeatureGraphNode targetNode;
    EdgeWeight weight;
  };
  typedef vector<RoutePathCross> CheckedPathT;
  class LastCrossFinder;

  static EdgeWeight getPathWeight(CheckedPathT const & path)
  {
    return accumulate(path.begin(), path.end(), 0, [](EdgeWeight sum, RoutePathCross const & elem){return sum+elem.weight;});
  }

  struct PathChecker
  {
    bool operator() (CheckedPathT const & a, CheckedPathT const & b) const {
      // Backward sorting order
      return getPathWeight(b)<getPathWeight(a);
    }
  };

  typedef priority_queue<CheckedPathT, vector<CheckedPathT>, PathChecker> RoutingTaskQueueT;

  /*!
   * \brief Makes route (points turns and other annotations) and submits it to @route class
   * \warning monitors m_requestCancel flag for process interrupting.
   * \param path vector of pathes through mwms
   * \param route class to render final route
   * \return NoError or error code
   */
  ResultCode MakeRouteFromCrossesPath(CheckedPathT const & path, Route & route);

  NodeID GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData, RoutingMappingPtrT const & routingMapping);
  void GetPossibleTurns(NodeID node, m2::PointD const & p1, m2::PointD const & p,
                        RoutingMappingPtrT const & routingMapping,
                        turns::TTurnCandidates & candidates);
  void GetTurnDirection(RawPathData const & node1,
                        RawPathData const & node2,
                        RoutingMappingPtrT const & routingMapping,
                        TurnItem & turn);
  m2::PointD GetPointForTurnAngle(OsrmMappingTypes::FtSeg const & seg,
                                  FeatureType const & ft, m2::PointD const & turnPnt,
                                  size_t (*GetPndInd)(const size_t, const size_t, const size_t)) const;
  /*!
   * \param junctionPoint is a point of the junction.
   * \param ingoingPointOneSegment is a point one segment before the junction along the route.
   * \param mapping is a route mapping.
   * \return number of all the segments which joins junctionPoint. That means
   * the number of ingoing segments plus the number of outgoing segments.
   * \warning NumberOfIngoingAndOutgoingSegments should be used carefully because
   * it's a time-consuming function.
   * \warning In multilevel crossroads there is an insignificant possibility that the returned value
   * contains redundant segments of roads of different levels.
   */
  size_t NumberOfIngoingAndOutgoingSegments(m2::PointD const & junctionPoint,
                                            m2::PointD const & ingoingPointOneSegment,
                                            RoutingMappingPtrT const & mapping) const;
  /*!
   * \brief GetTurnGeometry looks for all the road network edges near ingoingPoint.
   * GetTurnGeometry fills candidates with angles of all the incoming and outgoint segments.
   * \warning GetTurnGeometry should be used carefully because it's a time-consuming function.
   * \warning In multilevel crossroads there is an insignificant possibility that candidates
   * is filled with redundant segments of roads of different levels.
   */
  void GetTurnGeometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                       GeomTurnCandidateT & candidates, RoutingMappingPtrT const & mapping) const;

  Index const * m_pIndex;

  FeatureGraphNodeVecT graphNodes;

  FeatureGraphNodeVecT m_CachedTargetTask;
  m2::PointD m_CachedTargetPoint;

  RoutingIndexManager m_indexManager;

  m2::PointD m_startPt, m_finalPt, m_startDr;
  FeatureGraphNodeVecT m_cachedFinalNodes;
};
}  // namespace routing
