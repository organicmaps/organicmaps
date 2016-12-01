#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

namespace df
{

RouteBuilder::RouteBuilder(TFlushRouteFn const & flushRouteFn,
                           TFlushRouteSignFn const & flushRouteSignFn,
                           TFlushRouteArrowsFn const & flushRouteArrowsFn)
  : m_flushRouteFn(flushRouteFn)
  , m_flushRouteSignFn(flushRouteSignFn)
  , m_flushRouteArrowsFn(flushRouteArrowsFn)
{}

void RouteBuilder::Build(m2::PolylineD const & routePolyline, vector<double> const & turns,
                         df::ColorConstant color, vector<traffic::SpeedGroup> const & traffic,
                         df::RoutePattern const & pattern, ref_ptr<dp::TextureManager> textures,
                         int recacheId)
{
  drape_ptr<RouteData> routeData = make_unique_dp<RouteData>();
  routeData->m_routeIndex = m_routeIndex++;
  routeData->m_color = color;
  routeData->m_sourcePolyline = routePolyline;
  routeData->m_sourceTurns = turns;
  routeData->m_traffic = traffic;
  routeData->m_pattern = pattern;
  routeData->m_pivot = routePolyline.GetLimitRect().Center();
  routeData->m_recacheId = recacheId;
  RouteShape::CacheRoute(textures, *routeData.get());
  m_routeCache.insert(make_pair(routeData->m_routeIndex, routePolyline));

  // Flush route geometry.
  GLFunctions::glFlush();

  if (m_flushRouteFn != nullptr)
    m_flushRouteFn(move(routeData));
}

void RouteBuilder::ClearRouteCache()
{
  m_routeCache.clear();
}

void RouteBuilder::BuildSign(m2::PointD const & pos, bool isStart, bool isValid,
                             ref_ptr<dp::TextureManager> textures, int recacheId)
{
  drape_ptr<RouteSignData> routeSignData = make_unique_dp<RouteSignData>();
  routeSignData->m_isStart = isStart;
  routeSignData->m_position = pos;
  routeSignData->m_isValid = isValid;
  routeSignData->m_recacheId = recacheId;
  if (isValid)
    RouteShape::CacheRouteSign(textures, *routeSignData.get());

  // Flush route sign geometry.
  GLFunctions::glFlush();

  if (m_flushRouteSignFn != nullptr)
    m_flushRouteSignFn(move(routeSignData));
}

void RouteBuilder::BuildArrows(int routeIndex, vector<ArrowBorders> const & borders,
                               ref_ptr<dp::TextureManager> textures, int recacheId)
{
  auto it = m_routeCache.find(routeIndex);
  if (it == m_routeCache.end())
    return;

  drape_ptr<RouteArrowsData> routeArrowsData = make_unique_dp<RouteArrowsData>();
  routeArrowsData->m_pivot = it->second.GetLimitRect().Center();
  routeArrowsData->m_recacheId = recacheId;
  RouteShape::CacheRouteArrows(textures, it->second, borders, *routeArrowsData.get());

  // Flush route arrows geometry.
  GLFunctions::glFlush();

  if (m_flushRouteArrowsFn != nullptr)
    m_flushRouteArrowsFn(move(routeArrowsData));
}

} // namespace df
