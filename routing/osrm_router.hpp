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

namespace routing
{

class OsrmRouter : public IRouter
{
  typedef function<string (m2::PointD const &)> CountryFileFnT;
  CountryFileFnT m_countryFn;

public:

  struct FeatureGraphNode
  {
    PhantomNode m_node;
    OsrmFtSegMapping::FtSeg m_seg;
    m2::PointD m_segPt;
  };
  typedef vector<FeatureGraphNode> FeatureGraphNodeVecT;

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
  IRouter::ResultCode FindPhantomNodes(string const & fName, m2::PointD const & startPt, const m2::PointD &startDr, m2::PointD const & finalPt,
                                       FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId);

  bool NeedReload(string const & fPath) const;

  void CalculateRouteAsync(ReadyCallback const & callback);
  ResultCode CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & startDr, m2::PointD const & finalPt, Route & route);

private:
  NodeID GetTurnTargetNode(NodeID src, NodeID trg, QueryEdge::EdgeData const & edgeData);
  void GetPossibleTurns(NodeID node,
                        m2::PointD const & p1,
                        m2::PointD const & p,
                        uint32_t mwmId,
                        TurnCandidatesT & candidates);
  void GetTurnDirection(PathData const & node1,
                        PathData const & node2,
                        uint32_t mwmId, Route::TurnItem & turn, string const & fName);
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

  typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT m_dataFacade;
  OsrmFtSegMapping m_mapping;

  FilesMappingContainer m_container;

  bool m_isFinalChanged;
  m2::PointD m_startPt, m_finalPt, m_startDr;
  FeatureGraphNodeVecT m_cachedFinalNodes;

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  volatile bool m_requestCancel;
};

}
