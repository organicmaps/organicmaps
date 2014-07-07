#include "routing_engine.hpp"
#include "route.hpp"
#include "helicopter_router.hpp"
#include "osrm_router.hpp"

#include "../base/stl_add.hpp"
#include "../base/logging.hpp"

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
  if (!FindRouter(name))
  {
    if (name == "helicopter")
      m_routers.push_back(new HelicopterRouter);
    else if (name == "osrm")
      m_routers.push_back(new OsrmRouter);
  }
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

bool RoutingEngine::IsRoutingEnabled() const
{
  return !m_routers.empty();
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
  if (name == "all")
  {
    for (size_t i = 0; i < m_routers.size(); ++i)
      Calculate(m_routers[i]->GetName(), callback);
    return;
  }

  IRouter * p = FindRouter(name);
  if (!p)
  {
    LOG(LWARNING, ("Can't calculate route - router engine", name, "is not initialized."));
    return;
  }

  if (m_pointStates[0] == INVALID || m_pointStates[1] == INVALID)
  {
    LOG(LINFO, ("Routing calculation cancelled - start and/or end points are not initialized."));
    return;
  }

  if (m_pointStates[0] != MODIFIED || m_pointStates[1] != MODIFIED)
  {
    LOG(LINFO, ("Routing calculation cancelled - start and end points are the same."));
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

} // namespace routing
