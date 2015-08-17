#pragma once

#include "drape_frontend/route_shape.hpp"

#include "drape/pointers.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/polyline2d.hpp"

#include "std/function.hpp"

namespace df
{

class RouteBuilder
{
public:
  using TFlushRouteFn = function<void(drape_ptr<RouteData> &&)>;

  RouteBuilder(TFlushRouteFn const & flushRouteFn);

  void Build(m2::PolylineD const & routePolyline, vector<double> const & turns,
             dp::Color const & color, ref_ptr<dp::TextureManager> textures);

private:
  TFlushRouteFn m_flushRouteFn;
};

} // namespace df
