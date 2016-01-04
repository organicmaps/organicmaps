#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

namespace df
{

RouteBuilder::RouteBuilder(TFlushRouteFn const & flushRouteFn,
                           TFlushRouteSignFn const & flushRouteSignFn)
  : m_flushRouteFn(flushRouteFn)
  , m_flushRouteSignFn(flushRouteSignFn)
{}

void RouteBuilder::Build(m2::PolylineD const & routePolyline, vector<double> const & turns,
                         df::ColorConstant color, ref_ptr<dp::TextureManager> textures)
{
  CommonViewParams params;
  params.m_minVisibleScale = 1;
  params.m_rank = 0;
  params.m_depth = 0.0f;

  drape_ptr<RouteData> routeData = make_unique_dp<RouteData>();
  routeData->m_color = color;
  routeData->m_sourcePolyline = routePolyline;
  routeData->m_sourceTurns = turns;
  RouteShape(params).Draw(textures, *routeData.get());

  // Flush route geometry.
  GLFunctions::glFlush();

  if (m_flushRouteFn != nullptr)
    m_flushRouteFn(move(routeData));
}

void RouteBuilder::BuildSign(m2::PointD const & pos, bool isStart, bool isValid,
                             ref_ptr<dp::TextureManager> textures)
{
  drape_ptr<RouteSignData> routeSignData = make_unique_dp<RouteSignData>();
  routeSignData->m_isStart = isStart;
  routeSignData->m_position = pos;
  routeSignData->m_isValid = isValid;
  if (isValid)
  {
    CommonViewParams params;
    params.m_minVisibleScale = 1;
    params.m_rank = 0;
    params.m_depth = 0.0f;
    RouteShape(params).CacheRouteSign(textures, *routeSignData.get());
  }

  // Flush route sign geometry.
  GLFunctions::glFlush();

  if (m_flushRouteSignFn != nullptr)
    m_flushRouteSignFn(move(routeSignData));
}

} // namespace df
