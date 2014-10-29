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
  virtual void CalculateRoute(m2::PointD const & startPt, ReadyCallback const & callback);

protected:
  IRouter::ResultCode FindPhantomNodes(string const & fName, m2::PointD const & startPt, m2::PointD const & finalPt,
                                       FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId);

  bool NeedReload(string const & fPath) const;

  void CalculateRouteAsync(ReadyCallback const & callback);
  ResultCode CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & finalPt, Route & route);

  Route::TurnInstruction GetTurnInstruction(feature::TypesHolder const & ft1, feature::TypesHolder const & ft2,
                                            m2::PointD const & p1, m2::PointD const & p, m2::PointD const & p2,
                                            bool isStreetEqual) const;

private:
  Index const * m_pIndex;

  typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT m_dataFacade;
  OsrmFtSegMapping m_mapping;

  FilesMappingContainer m_container;

  bool m_isFinalChanged;
  m2::PointD m_startPt, m_finalPt;
  FeatureGraphNodeVecT m_cachedFinalNodes;

  threads::Mutex m_paramsMutex;
  threads::Mutex m_routeMutex;
  atomic_flag m_isReadyThread;

  volatile bool m_requestCancel;
};

}
