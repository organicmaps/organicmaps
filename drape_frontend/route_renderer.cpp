#include "drape_frontend/route_renderer.hpp"

#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/projection.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

float const halfWidthInPixel[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 2.0f, 2.0f, 2.0f, 2.0f,
  //11   12    13    14    15     16    17      18     19
  2.0f, 2.5f, 3.5f, 5.0f, 7.5f, 10.0f, 14.0f, 18.0f, 36.0f,
};

int const arrowAppearingZoomLevel = 14;

int const arrowPartsCount = 3;
double const arrowHeightFactor = 96.0 / 36.0;
double const arrowAspect = 400.0 / 192.0;
double const arrowTailSize = 20.0 / 400.0;
double const arrowHeadSize = 124.0 / 400.0;

int CheckForIntersection(double start, double end, vector<RouteSegment> const & segments)
{
  for (size_t i = 0; i < segments.size(); i++)
  {
    if (segments[i].m_isAvailable)
      continue;

    if ((start >= segments[i].m_start && start <= segments[i].m_end) ||
        (end >= segments[i].m_start && end <= segments[i].m_end) ||
        (start < segments[i].m_start && end > segments[i].m_end))
      return i;
  }
  return -1;
}

int FindNearestAvailableSegment(double start, double end, vector<RouteSegment> const & segments)
{
  double const threshold = 0.8;

  // check if distance intersects unavailable segment
  int index = CheckForIntersection(start, end, segments);

  // find nearest available segment if necessary
  if (index != -1)
  {
    double const len = end - start;
    for (int i = index; i < (int)segments.size(); i++)
    {
      double const factor = (segments[i].m_end - segments[i].m_start) / len;
      if (segments[i].m_isAvailable && factor > threshold)
        return (int)i;
    }
  }

  return -1;
}

void MergeAndClipBorders(vector<ArrowBorders> & borders)
{
  if (borders.empty())
    return;

  // mark groups
  for (size_t i = 0; i < borders.size() - 1; i++)
  {
    if (borders[i].m_endDistance >= borders[i + 1].m_startDistance)
      borders[i + 1].m_groupIndex = borders[i].m_groupIndex;
  }

  // merge groups
  int lastGroup = 0;
  size_t lastGroupIndex = 0;
  for (size_t i = 1; i < borders.size(); i++)
  {
    if (borders[i].m_groupIndex != lastGroup)
    {
      borders[lastGroupIndex].m_endDistance = borders[i - 1].m_endDistance;
      lastGroupIndex = i;
      lastGroup = borders[i].m_groupIndex;
    }
    else
    {
      borders[i].m_groupIndex = -1;
    }
  }
  borders[lastGroupIndex].m_endDistance = borders.back().m_endDistance;

  // clip groups
  auto const iter = remove_if(borders.begin(), borders.end(), [](ArrowBorders const & borders)
  {
    return borders.m_groupIndex == -1;
  });
  borders.erase(iter, borders.end());
}

}

RouteGraphics::RouteGraphics(dp::GLState const & state,
                             drape_ptr<dp::VertexArrayBuffer> && buffer,
                             dp::Color const & color)
  : m_state(state)
  , m_buffer(move(buffer))
  , m_color(color)
{}

RouteRenderer::RouteRenderer()
  : m_distanceFromBegin(0.0)
  , m_endOfRouteState(0, dp::GLState::OverlayLayer)
{}

void RouteRenderer::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                           dp::UniformValuesStorage const & commonUniforms)
{
  // half width calculation
  float halfWidth = 0.0;
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / log(2.0)), 1.0, scales::UPPER_STYLE_SCALE);
  double const truncedZoom = trunc(zoomLevel);
  int const index = truncedZoom - 1.0;
  float const lerpCoef = zoomLevel - truncedZoom;

  if (index < scales::UPPER_STYLE_SCALE - 1)
    halfWidth = halfWidthInPixel[index] + lerpCoef * (halfWidthInPixel[index + 1] - halfWidthInPixel[index]);
  else
    halfWidth = halfWidthInPixel[index];

  if (!m_routeGraphics.empty())
  {
    dp::GLState const & state = m_routeGraphics.front().m_state;

    // set up uniforms
    dp::UniformValuesStorage uniformStorage;
    glsl::vec4 color = glsl::ToVec4(m_routeGraphics.front().m_color);
    uniformStorage.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
    uniformStorage.SetFloatValue("u_halfWidth", halfWidth, halfWidth * screen.GetScale());
    uniformStorage.SetFloatValue("u_clipLength", m_distanceFromBegin);

    // set up shaders and apply uniforms
    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(state.GetProgramIndex());
    prg->Bind();
    dp::ApplyBlending(state, prg);
    dp::ApplyUniforms(commonUniforms, prg);
    dp::ApplyUniforms(uniformStorage, prg);

    // render routes
    for (RouteGraphics & graphics : m_routeGraphics)
    {
      ASSERT(graphics.m_state == state, ());
      graphics.m_buffer->Render();
    }

    // render arrows
    if (truncedZoom >= arrowAppearingZoomLevel)
    {
      // set up shaders and apply common uniforms
      ref_ptr<dp::GpuProgram> prgArrow = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
      prgArrow->Bind();
      dp::ApplyState(state, prgArrow);
      dp::ApplyUniforms(commonUniforms, prgArrow);

      for (RouteGraphics & graphics : m_routeGraphics)
        RenderArrow(prgArrow, graphics, halfWidth, screen);
    }
  }

  // render end of route
  if (m_endOfRouteBuffer != nullptr)
  {
    dp::UniformValuesStorage uniforms = commonUniforms;
    uniforms.SetFloatValue("u_opacity", 1.0);
    ref_ptr<dp::GpuProgram> eorProgram = mng->GetProgram(m_endOfRouteState.GetProgramIndex());
    eorProgram->Bind();
    dp::ApplyState(m_endOfRouteState, eorProgram);
    dp::ApplyUniforms(uniforms, eorProgram);
    m_endOfRouteBuffer->Render();
  }
}

void RouteRenderer::RenderArrow(ref_ptr<dp::GpuProgram> prg, RouteGraphics const & graphics,
                                float halfWidth, ScreenBase const & screen)
{
  double const arrowHalfWidth = halfWidth * arrowHeightFactor;
  double const glbArrowHalfWidth = arrowHalfWidth * screen.GetScale();
  double const arrowSize = 0.001;
  double const textureWidth = 2.0 * arrowHalfWidth * arrowAspect;

  dp::UniformValuesStorage uniformStorage;
  uniformStorage.SetFloatValue("u_halfWidth", arrowHalfWidth, glbArrowHalfWidth);
  uniformStorage.SetFloatValue("u_textureRect", m_routeData.m_arrowTextureRect.minX(),
                               m_routeData.m_arrowTextureRect.minY(),
                               m_routeData.m_arrowTextureRect.maxX(),
                               m_routeData.m_arrowTextureRect.maxY());

  // calculate arrows
  m_arrowBorders.clear();
  CalculateArrowBorders(arrowSize, screen.GetScale(), textureWidth, glbArrowHalfWidth);

  // split arrow's data by 16-elements buckets
  array<float, 16> borders;
  borders.fill(0.0f);
  size_t const elementsCount = borders.size();

  size_t index = 0;
  for (size_t i = 0; i < m_arrowBorders.size(); i++)
  {
    borders[index++] = m_arrowBorders[i].m_startDistance;
    borders[index++] = m_arrowBorders[i].m_startTexCoord;
    borders[index++] = m_arrowBorders[i].m_endDistance;
    borders[index++] = m_arrowBorders[i].m_endTexCoord;

    // render arrow's parts
    if (index == elementsCount || i == m_arrowBorders.size() - 1)
    {
      index = 0;
      uniformStorage.SetMatrix4x4Value("u_arrowBorders", borders.data());
      borders.fill(0.0f);

      dp::ApplyUniforms(uniformStorage, prg);
      graphics.m_buffer->Render();
    }
  }
}

void RouteRenderer::AddRouteRenderBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                                         RouteData const & routeData, ref_ptr<dp::GpuProgramManager> mng)
{
  m_routeData = routeData;

  m_routeGraphics.push_back(RouteGraphics());
  RouteGraphics & route = m_routeGraphics.back();

  route.m_state = state;
  route.m_color = m_routeData.m_color;
  route.m_buffer = bucket->MoveBuffer();
  route.m_buffer->Build(mng->GetProgram(route.m_state.GetProgramIndex()));
}

void RouteRenderer::AddEndOfRouteRenderBucket(dp::GLState const & state, drape_ptr<dp::RenderBucket> && bucket,
                                              ref_ptr<dp::GpuProgramManager> mng)
{
  m_endOfRouteState = state;
  m_endOfRouteBuffer = bucket->MoveBuffer();
  m_endOfRouteBuffer->Build(mng->GetProgram(m_endOfRouteState.GetProgramIndex()));
}

void RouteRenderer::Clear()
{
  m_routeGraphics.clear();
  m_endOfRouteBuffer.reset();
  m_arrowBorders.clear();
  m_routeSegments.clear();
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::ApplyJoinsBounds(double joinsBoundsScalar, double glbHeadLength)
{
  m_routeSegments.clear();
  m_routeSegments.reserve(2 * m_routeData.m_joinsBounds.size() + 1);

  // construct route's segments
  m_routeSegments.emplace_back(0.0, 0.0, true /* m_isAvailable */);
  for (size_t i = 0; i < m_routeData.m_joinsBounds.size(); i++)
  {
    double const start = m_routeData.m_joinsBounds[i].m_offset +
                         m_routeData.m_joinsBounds[i].m_start * joinsBoundsScalar;
    double const end = m_routeData.m_joinsBounds[i].m_offset +
                       m_routeData.m_joinsBounds[i].m_end * joinsBoundsScalar;

    m_routeSegments.back().m_end = start;
    m_routeSegments.emplace_back(start, end, false /* m_isAvailable */);

    m_routeSegments.emplace_back(end, 0.0, true /* m_isAvailable */);
  }
  m_routeSegments.back().m_end = m_routeData.m_length;

  // shift head of arrow if necessary
  bool needMerge = false;
  for (size_t i = 0; i < m_arrowBorders.size(); i++)
  {
    int headIndex = FindNearestAvailableSegment(m_arrowBorders[i].m_endDistance - glbHeadLength,
                                                m_arrowBorders[i].m_endDistance, m_routeSegments);
    if (headIndex != -1)
    {
      m_arrowBorders[i].m_endDistance = min(m_routeData.m_length, m_routeSegments[headIndex].m_start + glbHeadLength);
      needMerge = true;
    }
  }

  // merge intersected borders
  if (needMerge)
    MergeAndClipBorders(m_arrowBorders);
}

void RouteRenderer::CalculateArrowBorders(double arrowLength, double scale, double arrowTextureWidth, double joinsBoundsScalar)
{
  if (m_routeData.m_turns.empty())
    return;

  double halfLen = 0.5 * arrowLength;
  double const glbTextureWidth = arrowTextureWidth * scale;
  double const glbTailLength = arrowTailSize * glbTextureWidth;
  double const glbHeadLength = arrowHeadSize * glbTextureWidth;

  m_arrowBorders.reserve(m_routeData.m_turns.size() * arrowPartsCount);

  double const halfTextureWidth = 0.5 * glbTextureWidth;
  if (halfLen < halfTextureWidth)
    halfLen = halfTextureWidth;

  // initial filling
  for (size_t i = 0; i < m_routeData.m_turns.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = (int)i;
    arrowBorders.m_startDistance = max(0.0, m_routeData.m_turns[i] - halfLen * 0.8);
    arrowBorders.m_endDistance = min(m_routeData.m_length, m_routeData.m_turns[i] + halfLen * 1.2);

    if (arrowBorders.m_startDistance < m_distanceFromBegin)
      continue;

    m_arrowBorders.push_back(arrowBorders);
  }

  // merge intersected borders and clip them
  MergeAndClipBorders(m_arrowBorders);

  // apply joins bounds to prevent draw arrow's head on a join
  ApplyJoinsBounds(joinsBoundsScalar, glbHeadLength);

  // divide to parts of arrow
  size_t const bordersSize = m_arrowBorders.size();
  for (size_t i = 0; i < bordersSize; i++)
  {
    float const startDistance = m_arrowBorders[i].m_startDistance;
    float const endDistance = m_arrowBorders[i].m_endDistance;

    m_arrowBorders[i].m_endDistance = startDistance + glbTailLength;
    m_arrowBorders[i].m_startTexCoord = 0.0;
    m_arrowBorders[i].m_endTexCoord = arrowTailSize;

    ArrowBorders arrowHead;
    arrowHead.m_startDistance = endDistance - glbHeadLength;
    arrowHead.m_endDistance = endDistance;
    arrowHead.m_startTexCoord = 1.0 - arrowHeadSize;
    arrowHead.m_endTexCoord = 1.0;
    m_arrowBorders.push_back(arrowHead);

    ArrowBorders arrowBody;
    arrowBody.m_startDistance = m_arrowBorders[i].m_endDistance;
    arrowBody.m_endDistance = arrowHead.m_startDistance;
    arrowBody.m_startTexCoord = m_arrowBorders[i].m_endTexCoord;
    arrowBody.m_endTexCoord = arrowHead.m_startTexCoord;
    m_arrowBorders.push_back(arrowBody);
  }
}

} // namespace df

