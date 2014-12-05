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

  void GetTurnDirection(PathData const & node1,
                        PathData const & node2,
                        uint32_t mwmId, Route::TurnItem & turn);
  void FixupTurns(vector<m2::PointD> const & points, Route::TurnsT & turnsDir) const;
private:
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
