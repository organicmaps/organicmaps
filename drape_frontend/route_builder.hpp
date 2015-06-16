#pragma once

#include "drape_frontend/route_shape.hpp"

#include "drape/batcher.hpp"
#include "drape/glstate.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"
#include "drape/texture_manager.hpp"

#include "geometry/polyline2d.hpp"

#include "std/function.hpp"
#include "std/vector.hpp"

namespace df
{

struct RouteData
{
  dp::Color m_color;
  m2::RectF m_arrowTextureRect;
  vector<RouteJoinBounds> m_joinsBounds;
  double m_length;
  vector<double> m_turns;
};

class RouteBuilder
{
public:
  using TFlushRouteFn = function<void(dp::GLState const &, drape_ptr<dp::RenderBucket> &&, RouteData const &)>;

  RouteBuilder(TFlushRouteFn const & flushRouteFn);

  void Build(m2::PolylineD const & routePolyline, vector<double> const & turns,
             dp::Color const & color, ref_ptr<dp::TextureManager> textures);

private:
  TFlushRouteFn m_flushRouteFn;
  drape_ptr<dp::Batcher> m_batcher;
};

} // namespace df
