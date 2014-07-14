#pragma once

#include "router.hpp"

#include "../geometry/point2d.hpp"

#include "../std/vector.hpp"


namespace routing
{

class RoutingEngine
{
public:
  RoutingEngine();
  ~RoutingEngine();

  void AddRouter(string const & name);
  void AddRouter(IRouter * pRouter);
  void RemoveRouter(string const & name);
  bool IsRoutingEnabled() const;

  void SetStartingPoint(m2::PointD const & pt);
  void SetFinalPoint(m2::PointD const & pt);
  void Calculate(string const & name, IRouter::ReadyCallback const & callback);

private:
  IRouter * FindRouter(string const & name);

  m2::PointD m_points[2];

  enum PointState { INVALID, MODIFIED, CALCULATED };
  PointState m_pointStates[2];

  typedef vector<IRouter *> TRouters;
  TRouters m_routers;
};

} // namespace routing
