#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

namespace df
{

RouteBuilder::RouteBuilder(RouteBuilder::TFlushRouteFn const & flushRouteFn)
  : m_flushRouteFn(flushRouteFn)
{}

void RouteBuilder::Build(m2::PolylineD const & routePolyline,  vector<double> const & turns,
                         dp::Color const & color, ref_ptr<dp::TextureManager> textures)
{
  CommonViewParams params;
  params.m_depth = 0.0f;

  drape_ptr<RouteData> routeData = make_unique_dp<RouteData>();
  routeData->m_color = color;
  RouteShape(routePolyline, turns, params).Draw(textures, *routeData.get());

  if (m_flushRouteFn != nullptr)
    m_flushRouteFn(move(routeData));
}

} // namespace df
