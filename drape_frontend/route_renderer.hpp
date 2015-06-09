#pragma once

#include "drape/batcher.hpp"
#include "drape/glsl_types.hpp"
#include "drape/glstate.hpp"
#include "drape/gpu_program_manager.hpp"
#include "drape/pointers.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "platform/location.hpp"

namespace df
{

struct RouteGraphics
{
  RouteGraphics() : m_state(0, dp::GLState::GeometryLayer) {}
  RouteGraphics(dp::GLState const & state, drape_ptr<dp::VertexArrayBuffer> && buffer,
                dp::Color const & color);

  dp::GLState m_state;
  drape_ptr<dp::VertexArrayBuffer> m_buffer;
  dp::Color m_color;
};

class RouteRenderer final
{
public:
  RouteRenderer();

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

  void AddRouteRenderBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                            dp::Color const & color, ref_ptr<dp::GpuProgramManager> mng);

  void Clear();

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  vector<RouteGraphics> m_routeGraphics;
  double m_distanceFromBegin;
};

} // namespace df
