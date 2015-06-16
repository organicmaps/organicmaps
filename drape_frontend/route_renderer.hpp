#pragma once

#include "drape_frontend/route_builder.hpp"

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
                            RouteData const & routeData, ref_ptr<dp::GpuProgramManager> mng);

  void AddEndOfRouteRenderBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                                 ref_ptr<dp::GpuProgramManager> mng);

  void Clear();

  void UpdateDistanceFromBegin(double distanceFromBegin);

private:
  void CalculateArrowBorders(double arrowLength, double scale, double arrowTextureWidth,
                             double joinsBoundsScalar, vector<ArrowBorders> & borders);

  void ApplyJoinsBounds(double arrowTextureWidth, double joinsBoundsScalar, double glbTailLength,
                        double glbHeadLength, double scale, vector<ArrowBorders> & borders);

  void RenderArrow(RouteGraphics const & graphics, float halfWidth, ScreenBase const & screen,
                   ref_ptr<dp::GpuProgramManager> mng, dp::UniformValuesStorage const & commonUniforms);

  vector<RouteGraphics> m_routeGraphics;
  double m_distanceFromBegin;
  RouteData m_routeData;

  dp::GLState m_endOfRouteState;
  drape_ptr<dp::VertexArrayBuffer> m_endOfRouteBuffer;
};

} // namespace df
