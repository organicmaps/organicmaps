#include "routing_engine.hpp"
#include "route.hpp"

#include "../base/stl_add.hpp"


namespace routing
{

RoutingEngine::RoutingEngine()
{
  m_pointStates[0] = m_pointStates[1] = INVALID;
}

RoutingEngine::~RoutingEngine()
{
  for_each(m_routers.begin(), m_routers.end(), DeleteFunctor());
}

void RoutingEngine::AddRouter(string const & name)
{
  /// @todo
}

void RoutingEngine::RemoveRouter(string const & name)
{
  for (TRouters::iterator it = m_routers.begin(); it != m_routers.end(); ++it)
  {
    IRouter * router = *it;
    if (router->GetName() == name)
    {
      m_routers.erase(it);
      delete router;
      break;
    }
  }
}

void RoutingEngine::SetStartingPoint(m2::PointD const & pt)
{
  if (m_pointStates[0] == INVALID || !my::AlmostEqual(m_points[0], pt))
  {
    m_points[0] = pt;
    m_pointStates[0] = MODIFIED;
  }
}

void RoutingEngine::SetFinalPoint(m2::PointD const & pt)
{
  if (m_pointStates[1] == INVALID || !my::AlmostEqual(m_points[1], pt))
  {
    m_points[1] = pt;
    m_pointStates[1] = MODIFIED;
  }
}

void RoutingEngine::Calculate(string const & name, IRouter::ReadyCallback const & callback)
{
  IRouter * p = FindRouter(name);
  ASSERT(p, ());

  if (m_pointStates[0] == INVALID || m_pointStates[1] == INVALID)
  {
    // points are not initialized
    return;
  }

  if (m_pointStates[0] != MODIFIED || m_pointStates[1] != MODIFIED)
  {
    // nothing changed
    return;
  }

  if (m_pointStates[1] == MODIFIED)
    p->SetFinalPoint(m_points[1]);

  p->CalculateRoute(m_points[0], callback);
}

IRouter * RoutingEngine::FindRouter(string const & name)
{
  for (size_t i = 0; i < m_routers.size(); ++i)
    if (m_routers[i]->GetName() == name)
      return m_routers[i];
  return 0;
}

} // routing
