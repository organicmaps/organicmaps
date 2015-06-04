#pragma once

#include "drape/batcher.hpp"
#include "drape/glstate.hpp"
#include "drape/pointers.hpp"
#include "drape/render_bucket.hpp"

#include "geometry/polyline2d.hpp"

#include "std/function.hpp"

namespace df
{

class RouteBuilder
{
public:
  using TFlushRouteFn = function<void(dp::GLState const &,
                                      drape_ptr<dp::RenderBucket> &&,
                                      dp::Color const &)>;

  RouteBuilder(TFlushRouteFn const & flushRouteFn);

  void Build(m2::PolylineD const & routePolyline, dp::Color const & color);

private:
  TFlushRouteFn m_flushRouteFn;
  drape_ptr<dp::Batcher> m_batcher;
};

} // namespace df
