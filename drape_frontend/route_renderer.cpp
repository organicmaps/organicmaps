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

int const arrowPartsCount = 3;
double const arrowHeightFactor = 96.0 / 36.0;
double const arrowAspect = 400.0 / 192.0;
double const arrowTailSize = 20.0 / 400.0;
double const arrowHeadSize = 124.0 / 400.0;

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

int FindNearestAvailableSegment(bool isTail, double start, double end, vector<RouteSegment> const & segments)
{
  // check if distance is inside unavailable segment
  int startIndex = -1;
  int endIndex = -1;
  for (size_t i = 0; i < segments.size(); i++)
  {
    if (!segments[i].m_isAvailable && start >= segments[i].m_start && start <= segments[i].m_end)
      startIndex = i;

    if (!segments[i].m_isAvailable && end >= segments[i].m_start && end <= segments[i].m_end)
      endIndex = i;
  }

  // find nearest available segment if necessary
  int index = max(startIndex, endIndex);
  if (index != -1)
  {
    double const len = end - start;
    if (isTail)
    {
      for (int i = index; i >= 0; i--)
        if (segments[i].m_isAvailable && len <= (segments[i].m_end - segments[i].m_start))
          return (int)i;
    }
    else
    {
      for (int i = index; i < (int)segments.size(); i++)
        if (segments[i].m_isAvailable && len <= (segments[i].m_end - segments[i].m_start))
          return (int)i;
    }
  }

  return -1;
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

  for (RouteGraphics & graphics : m_routeGraphics)
  {
    dp::UniformValuesStorage uniformStorage;
    glsl::vec4 color = glsl::ToVec4(graphics.m_color);
    uniformStorage.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
    uniformStorage.SetFloatValue("u_halfWidth", halfWidth, halfWidth * screen.GetScale());
    uniformStorage.SetFloatValue("u_clipLength", m_distanceFromBegin);

    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(graphics.m_state.GetProgramIndex());
    prg->Bind();
    dp::ApplyBlending(graphics.m_state, prg);
    dp::ApplyUniforms(commonUniforms, prg);
    dp::ApplyUniforms(uniformStorage, prg);

    graphics.m_buffer->Render();

    // arrows rendering
    double arrowHalfWidth = halfWidth * arrowHeightFactor;
    uniformStorage.SetFloatValue("u_halfWidth", arrowHalfWidth, arrowHalfWidth * screen.GetScale());
    uniformStorage.SetFloatValue("u_textureRect", m_routeData.m_arrowTextureRect.minX(),
                                 m_routeData.m_arrowTextureRect.minY(),
                                 m_routeData.m_arrowTextureRect.maxX(),
                                 m_routeData.m_arrowTextureRect.maxY());

    m_turnPoints = { 0.0091, 0.0109 };
    double const textureWidth = 2.0 * arrowHalfWidth * arrowAspect;
    vector<ArrowBorders> arrowBorders;
    CalculateArrowBorders(0.001, screen.GetScale(), textureWidth, arrowHalfWidth * screen.GetScale(), arrowBorders);

    ref_ptr<dp::GpuProgram> prgArrow = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
    prgArrow->Bind();
    dp::ApplyState(graphics.m_state, prgArrow);
    dp::ApplyUniforms(commonUniforms, prgArrow);

    size_t const elementsCount = 16;
    vector<float> borders(elementsCount, 0.0);
    size_t index = 0;
    for (size_t i = 0; i < arrowBorders.size(); i++)
    {
      borders[index++] = arrowBorders[i].m_startDistance;
      borders[index++] = arrowBorders[i].m_startTexCoord;
      borders[index++] = arrowBorders[i].m_endDistance;
      borders[index++] = arrowBorders[i].m_endTexCoord;

      // fill rests by zeros
      if (i == arrowBorders.size() - 1)
      {
         for (size_t j = index; j < elementsCount; j++)
           borders[j] = 0.0;

         index = elementsCount;
      }

      // render arrow's parts
      if (index == elementsCount)
      {
        index = 0;
        uniformStorage.SetMatrix4x4Value("u_arrowBorders", borders.data());

        dp::ApplyUniforms(uniformStorage, prgArrow);
        graphics.m_buffer->Render();
      }
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

void RouteRenderer::Clear()
{
  m_routeGraphics.clear();
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::MergeAndClipBorders(vector<ArrowBorders> & borders, double scale, double arrowTextureWidth)
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
  auto const iter = remove_if(borders.begin(), borders.end(),
                              [&scale, &arrowTextureWidth](ArrowBorders const & borders)
  {
      if (borders.m_groupIndex == -1)
        return true;

      double const distanceInPixels = (borders.m_endDistance - borders.m_startDistance) * 0.9 / scale;
      return distanceInPixels < (arrowHeadSize + arrowTailSize) * arrowTextureWidth;
  });
  borders.erase(iter, borders.end());
}

void RouteRenderer::ApplyJoinsBounds(double arrowTextureWidth, double joinsBoundsScalar, double glbTailLength,
                                     double glbHeadLength, double scale, vector<ArrowBorders> & borders)
{
  vector<RouteSegment> segments;
  segments.reserve(2 * m_routeData.m_joinsBounds.size() + 1);

  // construct route's segments
  segments.emplace_back(0.0, 0.0, true /* m_isAvailable */);
  for (size_t i = 0; i < m_routeData.m_joinsBounds.size(); i++)
  {
    double const start = m_routeData.m_joinsBounds[i].m_offset +
                         m_routeData.m_joinsBounds[i].m_start * joinsBoundsScalar;
    double const end = m_routeData.m_joinsBounds[i].m_offset +
                       m_routeData.m_joinsBounds[i].m_end * joinsBoundsScalar;

    segments.back().m_end = start;
    segments.emplace_back(start, end, false /* m_isAvailable */);

    segments.emplace_back(end, 0.0, true /* m_isAvailable */);
  }
  segments.back().m_end = m_routeData.m_length;

  // shift tail and head of arrow if necessary
  bool needMerge = false;
  for (size_t i = 0; i < borders.size(); i++)
  {
    int tailIndex = FindNearestAvailableSegment(true /* isTail */, borders[i].m_startDistance,
                                                borders[i].m_startDistance + glbTailLength, segments);
    if (tailIndex != -1)
    {
      borders[i].m_startDistance = segments[tailIndex].m_end - glbTailLength;
      needMerge = true;
    }

    int headIndex = FindNearestAvailableSegment(false /* isTail */, borders[i].m_endDistance - glbHeadLength,
                                                borders[i].m_endDistance, segments);
    if (headIndex != -1)
    {
      borders[i].m_endDistance = segments[headIndex].m_start + glbHeadLength;
      needMerge = true;
    }
  }

  // merge intersected borders
  if (needMerge)
    MergeAndClipBorders(borders, scale, arrowTextureWidth);
}

void RouteRenderer::CalculateArrowBorders(double arrowLength, double scale, double arrowTextureWidth,
                                          double joinsBoundsScalar, vector<ArrowBorders> & borders)
{
  if (m_turnPoints.empty())
    return;

  double const halfLen = 0.5 * arrowLength;
  double const glbTextureWidth = arrowTextureWidth * scale;
  double const glbTailLength = arrowTailSize * glbTextureWidth;
  double const glbHeadLength = arrowHeadSize * glbTextureWidth;

  borders.reserve(m_turnPoints.size() * arrowPartsCount);

  // initial filling
  for (size_t i = 0; i < m_turnPoints.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = (int)i;
    arrowBorders.m_startDistance = m_turnPoints[i] - halfLen;
    arrowBorders.m_endDistance = m_turnPoints[i] + halfLen;
    borders.push_back(arrowBorders);
  }

  // merge intersected borders and clip them
  MergeAndClipBorders(borders, scale, arrowTextureWidth);

  // apply joins bounds to prevent draw arrow's head and tail on a join
  ApplyJoinsBounds(arrowTextureWidth, joinsBoundsScalar, glbTailLength,
                   glbHeadLength, scale, borders);

  // divide to parts of arrow
  size_t const bordersSize = borders.size();
  for (size_t i = 0; i < bordersSize; i++)
  {
    float const startDistance = borders[i].m_startDistance;
    float const endDistance = borders[i].m_endDistance;

    borders[i].m_endDistance = startDistance + glbTailLength;
    borders[i].m_startTexCoord = 0.0;
    borders[i].m_endTexCoord = arrowTailSize;

    ArrowBorders arrowHead;
    arrowHead.m_startDistance = endDistance - glbHeadLength;
    arrowHead.m_endDistance = endDistance;
    arrowHead.m_startTexCoord = 1.0 - arrowHeadSize;
    arrowHead.m_endTexCoord = 1.0;
    borders.push_back(arrowHead);

    ArrowBorders arrowBody;
    arrowBody.m_startDistance = borders[i].m_endDistance;
    arrowBody.m_endDistance = arrowHead.m_startDistance;
    arrowBody.m_startTexCoord = borders[i].m_endTexCoord;
    arrowBody.m_endTexCoord = arrowHead.m_startTexCoord;
    borders.push_back(arrowBody);
  }
}

} // namespace df

