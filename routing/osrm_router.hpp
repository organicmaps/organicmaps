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


namespace feature { class TypesHolder; }

struct RawRouteData;
struct PhantomNode;
struct PathData;
class FeatureType;

namespace routing
{
struct RoutePathCross;
using TCheckedPath = vector<RoutePathCross>;

typedef OsrmDataFacade<QueryEdge::EdgeData> TDataFacade;

/// All edges available for start route while routing
typedef vector<FeatureGraphNode> TFeatureGraphNodeVec;



class OsrmRouter : public IRouter
{
public:
  typedef vector<double> GeomTurnCandidateT;

  OsrmRouter(Index const * index, TCountryFileFn const & fn, RoutingVisualizerFn routingVisualization = nullptr);

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
  static bool FindRouteFromCases(TFeatureGraphNodeVec const & source,
                                 TFeatureGraphNodeVec const & target, TDataFacade & facade,
                                 RawRoutingResult & rawRoutingResult);

protected:
  IRouter::ResultCode FindPhantomNodes(string const & fName, m2::PointD const & point,
                                       m2::PointD const & direction, TFeatureGraphNodeVec & res,
                                       size_t maxCount, TRoutingMappingPtr const & mapping);

  /*!
   * \brief Compute turn and time estimation structs for OSRM raw route.
   * \param routingResult OSRM routing result structure to annotate.
   * \param mapping Feature mappings.
   * \param points Storage for unpacked points of the path.
   * \param turnsDir output turns annotation storage.
   * \param times output times annotation storage.
   * \param turnsGeom output turns geometry.
   * \return routing operation result code.
   */
  ResultCode MakeTurnAnnotation(RawRoutingResult const & routingResult,
                                TRoutingMappingPtr const & mapping, vector<m2::PointD> & points,
                                Route::TurnsT & turnsDir, Route::TimesT & times,
                                turns::TurnsGeomT & turnsGeom);

private:
  /*!
   * \brief Makes route (points turns and other annotations) from the map cross structs and submits them to @route class
   * \warning monitors m_requestCancel flag for process interrupting.
   * \param path vector of pathes through mwms
   * \param route class to render final route
   * \return NoError or error code
   */
  ResultCode MakeRouteFromCrossesPath(TCheckedPath const & path, Route & route);

  NodeID GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData, TRoutingMappingPtr const & routingMapping);
  void GetPossibleTurns(NodeID node, m2::PointD const & p1, m2::PointD const & p,
                        TRoutingMappingPtr const & routingMapping,
                        turns::TTurnCandidates & candidates);
  void GetTurnDirection(RawPathData const & node1, RawPathData const & node2,
                        TRoutingMappingPtr const & routingMapping, TurnItem & turn);
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
                                            TRoutingMappingPtr const & mapping) const;
  /*!
   * \brief GetTurnGeometry looks for all the road network edges near ingoingPoint.
   * GetTurnGeometry fills candidates with angles of all the incoming and outgoint segments.
   * \warning GetTurnGeometry should be used carefully because it's a time-consuming function.
   * \warning In multilevel crossroads there is an insignificant possibility that candidates
   * is filled with redundant segments of roads of different levels.
   */
  void GetTurnGeometry(m2::PointD const & junctionPoint, m2::PointD const & ingoingPoint,
                       GeomTurnCandidateT & candidates, TRoutingMappingPtr const & mapping) const;

  Index const * m_pIndex;

  TFeatureGraphNodeVec m_CachedTargetTask;
  m2::PointD m_CachedTargetPoint;

  RoutingIndexManager m_indexManager;
  RoutingVisualizerFn m_routingVisualization;
};
}  // namespace routing
