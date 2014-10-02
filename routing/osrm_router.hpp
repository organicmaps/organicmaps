#pragma once

#include "router.hpp"
#include "osrm2feature_map.hpp"
#include "osrm_data_facade.hpp"

#include "../std/function.hpp"

#include "../3party/osrm/osrm-backend/DataStructures/QueryEdge.h"


class Index;
struct PhantomNode;

namespace routing
{

class OsrmRouter : public IRouter
{

  m2::PointD m_finalPt;

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
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback);


protected:
  IRouter::ResultCode FindPhantomNodes(string const & fPath, m2::PointD const & startPt, m2::PointD const & finalPt,
                                       FeatureGraphNodeVecT & res, size_t maxCount, uint32_t & mwmId);

  bool NeedReload(string const & fPath) const;

  ResultCode CalculateRouteImpl(m2::PointD const & startPt, m2::PointD const & finalPt, Route & route);

private:
  Index const * m_pIndex;

  typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT m_dataFacade;
  OsrmFtSegMapping m_mapping;

  FilesMappingContainer m_container;

  FeatureGraphNodeVecT m_cachedFinalNodes;
};

}
