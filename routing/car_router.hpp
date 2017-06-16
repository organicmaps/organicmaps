#pragma once

#include "routing/index_router.hpp"
#include "routing/router.hpp"

#include "std/unique_ptr.hpp"
#include "std/vector.hpp"

class Index;

namespace routing
{
class CarRouter : public IRouter
{
public:
  typedef vector<double> GeomTurnCandidateT;

  CarRouter(Index & index, TCountryFileFn const & countryFileFn,
            unique_ptr<IndexRouter> localRouter);

  // IRouter overrides:
  virtual string GetName() const override { return "index-graph-car"; }

  ResultCode CalculateRoute(m2::PointD const & startPoint, m2::PointD const & startDirection,
                            m2::PointD const & finalPoint, RouterDelegate const & delegate,
                            Route & route) override;

  virtual void ClearState() override {}

private:
  bool AllMwmsHaveRoutingIndex() const;

  Index & m_index;
  unique_ptr<IndexRouter> m_router;
};
}  // namespace routing
