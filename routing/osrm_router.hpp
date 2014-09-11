#pragma once

#include "router.hpp"
#include "osrm_data_facade_types.hpp"

class Index;
class PhantomNode;

namespace routing
{

class OsrmRouter : public IRouter
{
  m2::PointD m_finalPt;

public:
  OsrmRouter(Index const * index);

  virtual string GetName() const;
  virtual void SetFinalPoint(m2::PointD const & finalPt);
  virtual void CalculateRoute(m2::PointD const & startingPt, ReadyCallback const & callback);


protected:
  bool FindPhantomNode(m2::PointD const & pt, PhantomNode & resultNode, uint32_t & mwmId);

private:
  Index const * m_pIndex;
  OsrmFtSegMapping m_mapping;
};


}
