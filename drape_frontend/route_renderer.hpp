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

struct ArrowBorders
{
  double m_startDistance = 0;
  double m_endDistance = 0;
  float m_startTexCoord = 0;
  float m_endTexCoord = 1;
  int m_groupIndex = 0;
};

class RouteRenderer final
{
public:
  RouteRenderer();

  void Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
              dp::UniformValuesStorage const & commonUniforms);

  void AddRouteRenderBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                            dp::Color const & color, m2::RectF const & arrowTextureRect,
                            ref_ptr<dp::GpuProgramManager> mng);

  void Clear();

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  void CalculateArrowBorders(double arrowLength, double scale,
                             float arrowTextureWidth, vector<ArrowBorders> & borders);

  vector<RouteGraphics> m_routeGraphics;
  double m_distanceFromBegin;
  m2::RectF m_arrowTextureRect;
  vector<double> m_turnPoints;
};

} // namespace df
