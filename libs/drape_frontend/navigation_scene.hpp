#pragma once

#include "drape_frontend/drape_engine.hpp"

#include <map>
#include <mutex>
#include <optional>
#include <set>

namespace df
{
class NavigationScene
{
public:
  void Attach(DrapeEngine * engine)
  {
    std::lock_guard lock(m_mutex);
    m_engines.insert(engine);
    for (auto const & [id, route] : m_routes)
      if (!route->IsAlternative())
        engine->AddSubrouteWithId(id, route);
    if (m_gps)
      engine->SetGpsInfo(*m_gps, m_navigable, m_matching);
  }
  void Detach(DrapeEngine * engine)
  {
    std::lock_guard lock(m_mutex);
    m_engines.erase(engine);
  }
  void UpdateMapStyle()
  {
    std::lock_guard lock(m_mutex);
    for (auto * engine : m_engines)
      engine->UpdateMapStyle();
  }
  void SetCompassInfo(location::CompassInfo const & info)
  {
    std::lock_guard lock(m_mutex);
    for (auto * engine : m_engines)
      engine->SetCompassInfo(info);
  }
  void InvalidateRect(m2::RectD const & rect)
  {
    std::lock_guard lock(m_mutex);
    for (auto * engine : m_engines)
      engine->InvalidateRect(rect);
  }
  void SetMapLangIndex(int8_t index)
  {
    std::lock_guard lock(m_mutex);
    for (auto * engine : m_engines)
      engine->SetMapLangIndex(index);
  }
  void Add(dp::DrapeID id, SubrouteConstPtr route)
  {
    std::lock_guard lock(m_mutex);
    m_routes[id] = route;
    if (!route->IsAlternative())
      for (auto * engine : m_engines)
        engine->AddSubrouteWithId(id, route);
  }
  void Remove(dp::DrapeID id)
  {
    std::lock_guard lock(m_mutex);
    if (id == 0)
    {
      // Zero means "all" in this scene, but RemoveSubroute(id, false) removes one ID.
      // Keep the cluster's fixed-position following mode when cancelling the route.
      for (auto const & [routeId, route] : m_routes)
        for (auto * engine : m_engines)
          engine->RemoveSubroute(routeId, false);
      m_routes.clear();
    }
    else
    {
      m_routes.erase(id);
      for (auto * engine : m_engines)
        engine->RemoveSubroute(id, false);
    }
  }
  void SetGpsInfo(location::GpsInfo const & gps, bool navigable, location::RouteMatchingInfo const & matching)
  {
    std::lock_guard lock(m_mutex);
    m_gps = gps;
    m_navigable = navigable;
    m_matching = matching;
    for (auto * engine : m_engines)
      engine->SetGpsInfo(gps, navigable, matching);
  }

private:
  std::mutex m_mutex;
  std::set<DrapeEngine *> m_engines;
  std::map<dp::DrapeID, SubrouteConstPtr> m_routes;
  std::optional<location::GpsInfo> m_gps;
  bool m_navigable = false;
  location::RouteMatchingInfo m_matching;
};
}  // namespace df
