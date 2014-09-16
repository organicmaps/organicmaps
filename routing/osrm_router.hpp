#pragma once

#include "router.hpp"
#include "osrm_data_facade_types.hpp"

#include "../std/function.hpp"


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
  OsrmFtSegMapping m_mapping;
};


}
