#pragma once

#include "route.hpp"
#include "router.hpp"
#include "osrm2feature_map.hpp"
#include "osrm_data_facade.hpp"

#include "../indexer/index.hpp"
#include "../base/mutex.hpp"

#include "../std/function.hpp"
#include "../std/numeric.hpp"
#include "../std/atomic.hpp"
#include "../std/queue.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/QueryEdge.h"
#include "../3party/osrm/osrm-backend/DataStructures/RawRouteData.h"

namespace feature { class TypesHolder; }

struct PhantomNode;
struct PathData;
class FeatureType;
struct RawRouteData;

namespace integration
{
  class OsrmRouterWrapper;
}

namespace routing
{

typedef function<string (m2::PointD const &)> CountryFileFnT;
typedef OsrmRawDataFacade<QueryEdge::EdgeData> RawDataFacadeT;
typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;

/// Single graph node representation for routing task
struct FeatureGraphNode
{
  PhantomNode m_node;
  OsrmFtSegMapping::FtSeg m_seg;
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


/// Datamapping and facade for single MWM and MWM.routing file
struct RoutingMapping
{
  DataFacadeT m_dataFacade;
  OsrmFtSegMapping m_segMapping;

  ///@param fName: mwm file path
  RoutingMapping(string const & fName, Index const * pIndex);

  ~RoutingMapping();

  void Map();

  void Unmap();

  void LoadFacade();

  void FreeFacade();

  bool IsValid() const {return m_isValid;}

  IRouter::ResultCode GetError() const {return m_error;}

  string GetName() const {return m_baseName;}

  Index::MwmId GetMwmId() const {return m_mwmId;}

private:
  size_t m_mapCounter;
  size_t m_facadeCounter;
  string m_baseName;
  FilesMappingContainer m_container;
  Index::MwmId m_mwmId;
  bool m_isValid;
  IRouter::ResultCode m_error;
};

typedef shared_ptr<RoutingMapping> RoutingMappingPtrT;

//! \brief The MappingGuard struct. Asks mapping to load all data on construction and free it on destruction
class MappingGuard
{
  RoutingMappingPtrT const m_mapping;

public:
  MappingGuard(RoutingMappingPtrT const mapping): m_mapping(mapping)
  {
    m_mapping->Map();
    m_mapping->LoadFacade();
  }

  ~MappingGuard()
  {
    m_mapping->Unmap();
    m_mapping->FreeFacade();
  }
};

/*! Manager for loading, cashing and building routing indexes.
 * Builds and shares special routing contexts.
*/
class RoutingIndexManager
{
  CountryFileFnT m_countryFn;

  map<string, RoutingMappingPtrT> m_mapping;

public:
  RoutingIndexManager(CountryFileFnT const & fn): m_countryFn(fn) {}

  RoutingMappingPtrT GetMappingByPoint(m2::PointD const & point, Index const * pIndex);

  RoutingMappingPtrT GetMappingByName(string const & fName, Index const * pIndex);

  friend class integration::OsrmRouterWrapper;
  typedef function<string (m2::PointD const &)> CountryFileFnT;
  CountryFileFnT m_countryFn;

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

  OsrmRouter(Index const * index, CountryFileFnT const & fn);

  virtual string GetName() const;
  virtual void ClearState();
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback, m2::PointD const & direction = m2::PointD::Zero());

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
   * \param taskNode output point task for router
   */
  static void GenerateRoutingTaskFromNodeId(const size_t nodeId, FeatureGraphNode & taskNode);

  void ActivateAdditionalFeatures() {m_additionalFeatures = true;}

protected:
  IRouter::ResultCode FindPhantomNodes(string const & fName, m2::PointD const & point, m2::PointD const & direction,
                                       FeatureGraphNodeVecT & res, size_t maxCount, RoutingMappingPtrT const  & mapping);

  /*!
   * \brief GetPointByNodeId finds geographical points for outgoing nodes to test linkage
   * \param node_id
   * \param routingMapping
   * \param use_start
   * \return point coordinates
   */
  m2::PointD GetPointByNodeId(const size_t node_id, RoutingMappingPtrT const & routingMapping, bool use_start);

  size_t FindNextMwmNode(RoutingMappingPtrT const & startMapping, size_t startId, RoutingMappingPtrT const & targetMapping);

  /*!
   * \brief Compute turn and time estimation structs for OSRM raw route.
   * \param routingResult OSRM routing result structure to annotate
   * \param mapping Feature mappings
   * \param points Unpacked point pathes
   * \param turnsDir output turns annotation storage
   * \param times output times annotation storage
   * \param turnsGeom output turns geometry
   * \return OSRM routing errors if any
   */
  ResultCode MakeTurnAnnotation(RawRoutingResultT const & routingResult, RoutingMappingPtrT const & mapping,
                                vector<m2::PointD> & points, Route::TurnsT & turnsDir,Route::TimesT & times, turns::TurnsGeomT & turnsGeom);

  void CalculateRouteAsync(ReadyCallback const & callback);
  ResultCode CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt, Route & route);

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
  m2::PointD GetPointForTurnAngle(OsrmFtSegMapping::FtSeg const & seg,
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

  bool m_isFinalChanged;
  m2::PointD m_startPt, m_finalPt, m_startDr;
  FeatureGraphNodeVecT m_cachedFinalNodes;

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  volatile bool m_requestCancel;

  // Additional features unlocking engine
  bool m_additionalFeatures;
};

}
