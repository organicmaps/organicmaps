#include "drape_frontend/route_builder.hpp"

#include "drape_frontend/route_shape.hpp"

namespace df
{

const int ESTIMATE_BUFFER_SIZE = 4000;

RouteBuilder::RouteBuilder(RouteBuilder::TFlushRouteFn const & flushRouteFn)
  : m_flushRouteFn(flushRouteFn)
  , m_batcher(make_unique_dp<dp::Batcher>(ESTIMATE_BUFFER_SIZE, ESTIMATE_BUFFER_SIZE))
{}

void RouteBuilder::Build(m2::PolylineD const & routePolyline, dp::Color const & color, ref_ptr<dp::TextureManager> textures)
{
  CommonViewParams params;
  params.m_depth = 0.0f;

  RouteShape shape(routePolyline, params);
  m2::RectF textureRect = shape.GetArrowTextureRect(textures);
  shape.PrepareGeometry();

  RouteData routeData;
  routeData.m_color = color;
  routeData.m_arrowTextureRect = textureRect;
  routeData.m_joinsBounds = shape.GetJoinsBounds();
  routeData.m_length = shape.GetLength();

  auto flushRoute = [this, &routeData](dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket)
  {
    if (m_flushRouteFn != nullptr)
      m_flushRouteFn(state, move(bucket), routeData);
  };

  m_batcher->StartSession(flushRoute);
  shape.Draw(make_ref(m_batcher), textures);
  m_batcher->EndSession();
}

} // namespace df
