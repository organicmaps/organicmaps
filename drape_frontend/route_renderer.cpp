#include "drape_frontend/route_renderer.hpp"

#include "drape/glsl_func.hpp"
#include "drape/utils/projection.hpp"

#include "base/logging.hpp"

namespace df
{

RouteGraphics::RouteGraphics(dp::GLState const & state,
                             drape_ptr<dp::VertexArrayBuffer> && buffer,
                             dp::Color const & color)
  : m_state(state)
  , m_buffer(move(buffer))
  , m_color(color)
{}

void RouteRenderer::Render(int currentZoomLevel, ref_ptr<dp::GpuProgramManager> mng,
                           dp::UniformValuesStorage const & commonUniforms)
{
  // TODO(@kuznetsov): calculate pixel width by zoom level
  float halfWidth = 30.0f;

  for (RouteGraphics & route : m_routes)
  {
    dp::UniformValuesStorage uniformStorage;
    glsl::vec4 color = glsl::ToVec4(route.m_color);
    uniformStorage.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
    uniformStorage.SetFloatValue("u_halfWidth", halfWidth);

    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(route.m_state.GetProgramIndex());
    prg->Bind();
    dp::ApplyState(route.m_state, prg);
    dp::ApplyUniforms(commonUniforms, prg);
    dp::ApplyUniforms(uniformStorage, prg);

    route.m_buffer->Render();
  }
}

void RouteRenderer::AddRoute(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                             dp::Color const & color, ref_ptr<dp::GpuProgramManager> mng)
{
  m_routes.push_back(RouteGraphics());
  RouteGraphics & route = m_routes.back();

  route.m_state = state;
  route.m_color = color;
  route.m_buffer = bucket->MoveBuffer();
  route.m_buffer->Build(mng->GetProgram(route.m_state.GetProgramIndex()));
}

} // namespace df
