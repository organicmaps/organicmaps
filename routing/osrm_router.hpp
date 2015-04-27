#pragma once

#include "routing/osrm2feature_map.hpp"
#include "routing/osrm_data_facade.hpp"
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

#include "3party/osrm/osrm-backend/DataStructures/QueryEdge.h"
#include "3party/osrm/osrm-backend/DataStructures/RawRouteData.h"

namespace feature { class TypesHolder; }

struct PhantomNode;
struct PathData;
class FeatureType;
struct RawRouteData;

namespace routing
{

typedef function<string (m2::PointD const &)> CountryFileFnT;
typedef OsrmRawDataFacade<QueryEdge::EdgeData> RawDataFacadeT;
typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;

/// Single graph node representation for routing task
struct FeatureGraphNode
{
  PhantomNode m_node;
  OsrmMappingTypes::FtSeg m_seg;
  m2::PointD m_segPt;
};
/// All edges available for start route while routing
typedef vector<FeatureGraphNode> FeatureGraphNodeVecT;
/// Points vector to calculate several routes
typedef vector<FeatureGraphNode> MultiroutingTaskPointT;

/*!
 * \brief The OSRM routing result struct. Contains raw routing result and iterators to source and target edges.
 * \property routePath: result path data
 * \property sourceEdge: iterator to src edge from source vector
 * \property targetEdge: iterator to target edge from target vector
 */
struct RawRoutingResultT
{
  RawRouteData m_routePath;
  FeatureGraphNode m_sourceEdge;
  FeatureGraphNode m_targetEdge;
};

typedef vector<RawRoutingResultT> MultipleRoutingResultT;

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

  struct TurnCandidate
  {
    double m_angle;
    NodeID m_node;

    TurnCandidate(double a, NodeID n)
      : m_angle(a), m_node(n)
    {
    }
  };
  typedef vector<TurnCandidate> TurnCandidatesT;

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
  static bool FindSingleRoute(FeatureGraphNodeVecT const & source, FeatureGraphNodeVecT const & target, DataFacadeT & facade,
                       RawRoutingResultT & rawRoutingResult);

  /*!
   * \brief FindWeightsMatrix Find weights matrix from sources to targets. WARNING it finds only weights, not pathes.
   * \param sources vector. Allows only one phantom node for one source. Each source is the start OSRM node.
   * \param targets vector. Allows only one phantom node for one target. Each target is the finish OSRM node.
   * \param facade osrm data facade reference
   * \param packed result vector with weights. Source nodes are rows.
   * cost(source1 -> target1) cost(source1 -> target2) cost(source2 -> target1) cost(source2 -> target2)
   */
  static void FindWeightsMatrix(MultiroutingTaskPointT const & sources, MultiroutingTaskPointT const & targets,
                                RawDataFacadeT & facade, vector<EdgeWeight> & result);

  /*!
   * \brief GenerateRoutingTaskFromNodeId fill taskNode with values for making route
   * \param nodeId osrm node idetifier
   * \param isStartNode true if this node will first in the path
   * \param taskNode output point task for router
   */
  static void GenerateRoutingTaskFromNodeId(const NodeID nodeId, bool const isStartNode,
                                            FeatureGraphNode & taskNode);

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
  ResultCode MakeTurnAnnotation(RawRoutingResultT const & routingResult,
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
  void GetPossibleTurns(NodeID node,
                        m2::PointD const & p1,
                        m2::PointD const & p,
                        RoutingMappingPtrT const & routingMapping,
                        TurnCandidatesT & candidates);
  void GetTurnDirection(PathData const & node1,
                        PathData const & node2,
                        RoutingMappingPtrT const & routingMapping,
                        Route::TurnItem & turn);
  void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TurnsT const & turnsDir, turns::TurnsGeomT & turnsGeom) const;
  void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir) const;
  m2::PointD GetPointForTurnAngle(OsrmMappingTypes::FtSeg const & seg,
                                  FeatureType const & ft, m2::PointD const & turnPnt,
                                  size_t (*GetPndInd)(const size_t, const size_t, const size_t)) const;
  turns::TurnDirection InvertDirection(turns::TurnDirection dir) const;
  turns::TurnDirection MostRightDirection(double angle) const;
  turns::TurnDirection MostLeftDirection(double angle) const;
  turns::TurnDirection IntermediateDirection(double angle) const;
  void GetTurnGeometry(m2::PointD const & p, m2::PointD const & p1,
                       OsrmRouter::GeomTurnCandidateT & candidates, RoutingMappingPtrT const & mapping) const;
  bool KeepOnewayOutgoingTurnIncomingEdges(Route::TurnItem const & turn,
                              m2::PointD const & p, m2::PointD const & p1, RoutingMappingPtrT const & mapping) const;
  bool KeepOnewayOutgoingTurnRoundabout(bool isRound1, bool isRound2) const;
  turns::TurnDirection RoundaboutDirection(bool isRound1, bool isRound2,
                                           bool hasMultiTurns, Route::TurnItem const & turn) const;

  Index const * m_pIndex;

  FeatureGraphNodeVecT graphNodes;

  FeatureGraphNodeVecT m_CachedTargetTask;
  m2::PointD m_CachedTargetPoint;

  RoutingIndexManager m_indexManager;

  m2::PointD m_startPt, m_finalPt, m_startDr;
  FeatureGraphNodeVecT m_cachedFinalNodes;
};

}
