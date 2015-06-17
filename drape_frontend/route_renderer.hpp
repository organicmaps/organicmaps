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

struct RouteSegment
{
  double m_start = 0;
  double m_end = 0;
  bool m_isAvailable = false;

  RouteSegment(double start, double end, bool isAvailable)
    : m_start(start)
    , m_end(end)
    , m_isAvailable(isAvailable)
  {}
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
  void CalculateArrowBorders(double arrowLength, double scale, double arrowTextureWidth, double joinsBoundsScalar);

  void ApplyJoinsBounds(double joinsBoundsScalar, double glbHeadLength);

  void RenderArrow(ref_ptr<dp::GpuProgram> prg, RouteGraphics const & graphics, float halfWidth, ScreenBase const & screen);

  vector<RouteGraphics> m_routeGraphics;
  double m_distanceFromBegin;
  RouteData m_routeData;

  dp::GLState m_endOfRouteState;
  drape_ptr<dp::VertexArrayBuffer> m_endOfRouteBuffer;

  vector<ArrowBorders> m_arrowBorders;
  vector<RouteSegment> m_routeSegments;
};

} // namespace df
