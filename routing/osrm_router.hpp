#pragma once

#include "route.hpp"
#include "router.hpp"
#include "osrm2feature_map.hpp"
#include "osrm_data_facade.hpp"

#include "../base/mutex.hpp"

#include "../std/function.hpp"
#include "../std/atomic.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/QueryEdge.h"

namespace feature { class TypesHolder; }

class Index;
struct PhantomNode;
struct PathData;
class FeatureType;
class RawRouteData;

namespace routing
{

typedef function<string (m2::PointD const &)> CountryFileFnT;
typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;

///Single graph node representation for routing task
struct FeatureGraphNode
{
  PhantomNode m_node;
  OsrmFtSegMapping::FtSeg m_seg;
  m2::PointD m_segPt;
};
/// All edges available for start route while routing
typedef vector<FeatureGraphNode> FeatureGraphNodeVecT;
/// Points vector to calculate several routes
typedef vector<FeatureGraphNodeVecT> MultiroutingTaskPointT;



///Datamapping and facade for single MWM and MWM.routing file
struct RoutingMapping
{
  DataFacadeT dataFacade;
  OsrmFtSegMapping mapping;

  ///@param fName: mwm file path
  RoutingMapping(string fName);

  ~RoutingMapping()
  {
    // Clear data while m_container is valid.
    dataFacade.Clear();
    mapping.Clear();
    m_container.Close();
  }

  void Map()
  {
    ++map_counter;
    if (!mapping.IsMapped())
      mapping.Map(m_container);
  }

  void Unmap()
  {
    --map_counter;
    if (map_counter<1 && mapping.IsMapped())
      mapping.Unmap();
  }

  void LoadFacade()
  {
    if (!facade_counter)
      dataFacade.Load(m_container);
    ++facade_counter;
  }

  void FreeFacade()
  {
    --facade_counter;
    if (!facade_counter)
      dataFacade.Clear();
  }

  string GetName() {return m_base_name;}

private:
  size_t map_counter;
  size_t facade_counter;
  string m_base_name;
  FilesMappingContainer m_container;
};

typedef shared_ptr<RoutingMapping> RoutingMappingPtrT;

/*! Manager for loading, cashing and building routing indexes.
 * Builds and shares special routing contexts.
*/
class RoutingIndexManager
{
  CountryFileFnT m_countryFn;

  std::map<string, RoutingMappingPtrT> m_mapping;

public:
  RoutingIndexManager(CountryFileFnT const & fn): m_countryFn(fn) {}

  RoutingMappingPtrT GetMappingByPoint(m2::PointD point)
  {
    string fName = m_countryFn(point);
    //Check if we have already load this file
    auto mapIter = m_mapping.find(fName);
    if (mapIter != m_mapping.end())
      return mapIter->second;
    //Or load and check file
    RoutingMappingPtrT new_mapping = RoutingMappingPtrT(new RoutingMapping(fName));

    m_mapping.insert(std::make_pair(fName, new_mapping));
    return new_mapping;
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

  typedef vector<double> GeomTurnCandidateT;

  OsrmRouter(Index const * index, CountryFileFnT const & fn);

  virtual string GetName() const;
  virtual void ClearState();
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback, m2::PointD const & direction = m2::PointD::Zero());

protected:
  IRouter::ResultCode FindPhantomNodes(string const & fName, m2::PointD const & point, const m2::PointD &direction,
                                       FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId, OsrmFtSegMapping const & mapping);

  /*! Find single shortest path in single MWM between 2 sets of edges
   * @param source: vector of source edgest to make path
   * @param taget: vector of target edges to make path
   * @param facade: OSRM routing data facade to recover graph information
   * @param outputPath: result path data
   * @param sourceEdge: iterator to src edge from source vector
   * @param sourceEdge: iterator to target edge from target vector
   * @return true if path exists. False otherwise
   */
  bool FindSingleRoute(FeatureGraphNodeVecT const & source, FeatureGraphNodeVecT const & target, DataFacadeT & facade,
                       RawRouteData& outputPath,
                       FeatureGraphNodeVecT::const_iterator & sourceEdge,
                       FeatureGraphNodeVecT::const_iterator & targetEdge);

  void CalculateRouteAsync(ReadyCallback const & callback);
  ResultCode CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt, Route & route);

private:
  NodeID GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData, RoutingMappingPtrT const & routingMapping);
  void GetPossibleTurns(NodeID node,
                        m2::PointD const & p1,
                        m2::PointD const & p,
                        uint32_t mwmId,
                        RoutingMappingPtrT const & routingMapping,
                        TurnCandidatesT & candidates);
  void GetTurnDirection(PathData const & node1,
                        PathData const & node2,
                        uint32_t mwmId,
                        RoutingMappingPtrT const & routingMapping,
                        Route::TurnItem & turn);
  void CalculateTurnGeometry(vector<m2::PointD> const & points, Route::TurnsT const & turnsDir, turns::TurnsGeomT & turnsGeom) const;
  void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir) const;
  m2::PointD GetPointForTurnAngle(OsrmFtSegMapping::FtSeg const &seg,
                                  FeatureType const &ft, m2::PointD const &turnPnt,
                                  size_t (*GetPndInd)(const size_t, const size_t, const size_t)) const;
  turns::TurnDirection InvertDirection(turns::TurnDirection dir) const;
  turns::TurnDirection MostRightDirection(double angle) const;
  turns::TurnDirection MostLeftDirection(double angle) const;
  turns::TurnDirection IntermediateDirection(double angle) const;
  void GetTurnGeometry(m2::PointD const & p, m2::PointD const & p1,
                       OsrmRouter::GeomTurnCandidateT & candidates, string const & fName) const;
  bool KeepOnewayOutgoingTurnIncomingEdges(TurnCandidatesT const & nodes, Route::TurnItem const & turn,
                              m2::PointD const & p, m2::PointD const & p1, string const & fName) const;
  bool KeepOnewayOutgoingTurnRoundabout(bool isRound1, bool isRound2) const;
  turns::TurnDirection RoundaboutDirection(bool isRound1, bool isRound2,
                                           bool hasMultiTurns, Route::TurnItem const & turn) const;

  Index const * m_pIndex;

  FeatureGraphNodeVecT graphNodes;

  MultiroutingTaskPointT m_CachedTargetTask;
  m2::PointD m_CachedTargetPoint;

  RoutingIndexManager m_indexManager;

  bool m_isFinalChanged;
  m2::PointD m_startPt, m_finalPt, m_startDr;
  FeatureGraphNodeVecT m_cachedFinalNodes;

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  volatile bool m_requestCancel;
};

}
