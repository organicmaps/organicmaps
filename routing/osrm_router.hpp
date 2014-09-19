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
  OsrmRouter(Index const * index, CountryFileFnT const & fn);

  virtual string GetName() const;
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback);


protected:
  bool FindPhantomNode(m2::PointD const & pt, PhantomNode & resultNode, uint32_t & mwmId,
                       OsrmFtSegMapping::FtSeg & seg, m2::PointD & segPt);

private:
  Index const * m_pIndex;

  typedef OsrmDataFacade<QueryEdge::EdgeData> DataFacadeT;
  DataFacadeT m_dataFacade;
  OsrmFtSegMapping m_mapping;
  string m_lastMwmName;

  FilesMappingContainer m_container;
};


}
