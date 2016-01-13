#pragma once

#include "drape_frontend/color_constants.hpp"
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
  using TFlushRouteSignFn = function<void(drape_ptr<RouteSignData> &&)>;

  RouteBuilder(TFlushRouteFn const & flushRouteFn,
               TFlushRouteSignFn const & flushRouteSignFn);

  void Build(m2::PolylineD const & routePolyline, vector<double> const & turns,
             df::ColorConstant color, ref_ptr<dp::TextureManager> textures);

  void BuildSign(m2::PointD const & pos, bool isStart, bool isValid,
                 ref_ptr<dp::TextureManager> textures);

private:
  TFlushRouteFn m_flushRouteFn;
  TFlushRouteSignFn m_flushRouteSignFn;
};

} // namespace df
