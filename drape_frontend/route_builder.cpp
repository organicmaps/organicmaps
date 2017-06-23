#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

namespace df
{
RouteBuilder::RouteBuilder(TFlushRouteFn const & flushRouteFn,
                           TFlushRouteArrowsFn const & flushRouteArrowsFn)
  : m_flushRouteFn(flushRouteFn)
  , m_flushRouteArrowsFn(flushRouteArrowsFn)
{}

void RouteBuilder::Build(dp::DrapeID subrouteId, drape_ptr<Subroute> && subroute,
                         ref_ptr<dp::TextureManager> textures, int recacheId)
{
  drape_ptr<RouteData> routeData = make_unique_dp<RouteData>();
  routeData->m_subrouteId = subrouteId;
  routeData->m_subroute = std::move(subroute);
  routeData->m_pivot = routeData->m_subroute->m_polyline.GetLimitRect().Center();
  routeData->m_recacheId = recacheId;
  RouteShape::CacheRoute(textures, *routeData.get());
  m_routeCache.insert(std::make_pair(subrouteId, routeData->m_subroute->m_polyline));

  // Flush route geometry.
  GLFunctions::glFlush();

  if (m_flushRouteFn != nullptr)
    m_flushRouteFn(std::move(routeData));
}

void RouteBuilder::ClearRouteCache()
{
  m_routeCache.clear();
}

void RouteBuilder::BuildArrows(dp::DrapeID subrouteId, std::vector<ArrowBorders> const & borders,
                               ref_ptr<dp::TextureManager> textures, int recacheId)
{
  auto it = m_routeCache.find(subrouteId);
  if (it == m_routeCache.end())
    return;

  drape_ptr<RouteArrowsData> routeArrowsData = make_unique_dp<RouteArrowsData>();
  routeArrowsData->m_subrouteId = subrouteId;
  routeArrowsData->m_pivot = it->second.GetLimitRect().Center();
  routeArrowsData->m_recacheId = recacheId;
  RouteShape::CacheRouteArrows(textures, it->second, borders, *routeArrowsData.get());

  // Flush route arrows geometry.
  GLFunctions::glFlush();

  if (m_flushRouteArrowsFn != nullptr)
    m_flushRouteArrowsFn(std::move(routeArrowsData));
}
}  // namespace df
