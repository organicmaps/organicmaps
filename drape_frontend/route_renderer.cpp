#include "drape_frontend/route_renderer.hpp"

#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/projection.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

double const kArrowHeightFactor = 96.0 / 36.0;
double const kArrowAspect = 400.0 / 192.0;
double const kArrowTailSize = 20.0 / 400.0;
double const kArrowHeadSize = 124.0 / 400.0;

float const kHalfWidthInPixel[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  2.0f, 2.0f, 3.0f, 3.0f, 3.0f, 4.0f, 4.0f, 4.0f, 5.0f, 5.0f,
  //11   12    13    14    15    16    17    18    19     20
  6.0f, 6.0f, 7.0f, 7.0f, 7.0f, 7.0f, 8.0f, 10.0f, 24.0f, 36.0f
};

uint8_t const kAlphaValue[] =
{
  //1   2    3    4    5    6    7    8    9    10
  204, 204, 204, 204, 204, 204, 204, 204, 204, 204,
  //11  12   13   14   15   16   17   18   19   20
  204, 204, 204, 204, 190, 180, 170, 160, 140, 120
};

int const kArrowAppearingZoomLevel = 14;

enum SegmentStatus
{
  OK = -1,
  NoSegment = -2
};

int const kInvalidGroup = -1;

// Checks for route segments for intersection with the distance [start; end].
int CheckForIntersection(double start, double end, vector<RouteSegment> const & segments)
{
  for (size_t i = 0; i < segments.size(); i++)
  {
    if (segments[i].m_isAvailable)
      continue;

    if (start <= segments[i].m_end && end >= segments[i].m_start)
      return i;
  }
  return SegmentStatus::OK;
}

// Finds the nearest appropriate route segment to the distance [start; end].
int FindNearestAvailableSegment(double start, double end, vector<RouteSegment> const & segments)
{
  double const kThreshold = 0.8;

  // check if distance intersects unavailable segment
  int index = CheckForIntersection(start, end, segments);
  if (index == SegmentStatus::OK)
    return SegmentStatus::OK;

  // find nearest available segment if necessary
  double const len = end - start;
  for (size_t i = index; i < segments.size(); i++)
  {
    double const factor = (segments[i].m_end - segments[i].m_start) / len;
    if (segments[i].m_isAvailable && factor > kThreshold)
      return static_cast<int>(i);
  }
  return SegmentStatus::NoSegment;
}

void ClipBorders(vector<ArrowBorders> & borders)
{
  auto invalidBorders = [](ArrowBorders const & borders)
  {
    return borders.m_groupIndex == kInvalidGroup;
  };
  borders.erase(remove_if(borders.begin(), borders.end(), invalidBorders), borders.end());
}

void MergeAndClipBorders(vector<ArrowBorders> & borders)
{
  // initial clipping
  ClipBorders(borders);

  if (borders.empty())
    return;

  // mark groups
  for (size_t i = 0; i + 1 < borders.size(); i++)
  {
    if (borders[i].m_endDistance >= borders[i + 1].m_startDistance)
      borders[i + 1].m_groupIndex = borders[i].m_groupIndex;
  }

  // merge groups
  int lastGroup = borders.front().m_groupIndex;
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
      borders[i].m_groupIndex = kInvalidGroup;
    }
  }
  borders[lastGroupIndex].m_endDistance = borders.back().m_endDistance;

  // clip groups
  ClipBorders(borders);
}

void BuildBuckets(RouteRenderProperty const & renderProperty, ref_ptr<dp::GpuProgramManager> mng)
{
  for (drape_ptr<dp::RenderBucket> const & bucket : renderProperty.m_buckets)
    bucket->GetBuffer()->Build(mng->GetProgram(renderProperty.m_state.GetProgramIndex()));
}

}

RouteRenderer::RouteRenderer()
  : m_distanceFromBegin(0.0)
{}

void RouteRenderer::InterpolateByZoom(ScreenBase const & screen, float & halfWidth, float & alpha, double & zoom) const
{
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / log(2.0)), 1.0, scales::UPPER_STYLE_SCALE + 1.0);
  zoom = trunc(zoomLevel);
  int const index = zoom - 1.0;
  float const lerpCoef = zoomLevel - zoom;

  if (index < scales::UPPER_STYLE_SCALE)
  {
    halfWidth = kHalfWidthInPixel[index] + lerpCoef * (kHalfWidthInPixel[index + 1] - kHalfWidthInPixel[index]);

    float const alpha1 = static_cast<float>(kAlphaValue[index]) / numeric_limits<uint8_t>::max();
    float const alpha2 = static_cast<float>(kAlphaValue[index + 1]) / numeric_limits<uint8_t>::max();
    alpha = alpha1 + lerpCoef * (alpha2 - alpha1);
  }
  else
  {
    halfWidth = kHalfWidthInPixel[scales::UPPER_STYLE_SCALE];
    alpha = static_cast<float>(kAlphaValue[scales::UPPER_STYLE_SCALE]) / numeric_limits<uint8_t>::max();
  }
}

void RouteRenderer::Render(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                           dp::UniformValuesStorage const & commonUniforms)
{
  if (!m_routeData)
    return;

  // interpolate values by zoom level
  double zoom = 0.0;
  float halfWidth = 0.0;
  float alpha = 0.0;
  InterpolateByZoom(screen, halfWidth, alpha, zoom);

  // render route
  {
    dp::GLState const & state = m_routeData->m_route.m_state;

    // set up uniforms
    dp::UniformValuesStorage uniformStorage;
    glsl::vec4 color = glsl::ToVec4(m_routeData->m_color);
    uniformStorage.SetFloatValue("u_color", color.r, color.g, color.b, alpha);
    uniformStorage.SetFloatValue("u_halfWidth", halfWidth, halfWidth * screen.GetScale());
    uniformStorage.SetFloatValue("u_clipLength", m_distanceFromBegin);

    // set up shaders and apply uniforms
    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(state.GetProgramIndex());
    prg->Bind();
    dp::ApplyBlending(state, prg);
    dp::ApplyUniforms(commonUniforms, prg);
    dp::ApplyUniforms(uniformStorage, prg);

    // render routes
    for (drape_ptr<dp::RenderBucket> const & bucket : m_routeData->m_route.m_buckets)
      bucket->Render(screen);
  }

  // render arrows
  if (zoom >= kArrowAppearingZoomLevel && !m_routeData->m_arrows.empty())
  {
    dp::GLState const & state = m_routeData->m_arrows.front()->m_arrow.m_state;

    // set up shaders and apply common uniforms
    dp::UniformValuesStorage uniforms = commonUniforms;
    uniforms.SetFloatValue("u_textureRect", m_routeData->m_arrowTextureRect.minX(),
                                            m_routeData->m_arrowTextureRect.minY(),
                                            m_routeData->m_arrowTextureRect.maxX(),
                                            m_routeData->m_arrowTextureRect.maxY());

    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
    prg->Bind();
    dp::ApplyState(state, prg);
    dp::ApplyUniforms(uniforms, prg);

    for (drape_ptr<ArrowRenderProperty> & property : m_routeData->m_arrows)
      RenderArrow(prg, property, halfWidth, screen);
  }

  // render end of route
  {
    dp::GLState const & state = m_routeData->m_endOfRouteSign.m_state;

    dp::UniformValuesStorage uniforms = commonUniforms;
    uniforms.SetFloatValue("u_opacity", 1.0);
    ref_ptr<dp::GpuProgram> program = mng->GetProgram(state.GetProgramIndex());
    program->Bind();
    dp::ApplyState(m_routeData->m_endOfRouteSign.m_state, program);
    dp::ApplyUniforms(uniforms, program);
    for (drape_ptr<dp::RenderBucket> const & bucket : m_routeData->m_endOfRouteSign.m_buckets)
      bucket->Render(screen);
  }
}

void RouteRenderer::RenderArrow(ref_ptr<dp::GpuProgram> prg, drape_ptr<ArrowRenderProperty> const & property,
                                float halfWidth, ScreenBase const & screen)
{
  double const arrowHalfWidth = halfWidth * kArrowHeightFactor;
  double const glbArrowHalfWidth = arrowHalfWidth * screen.GetScale();
  double const textureWidth = 2.0 * arrowHalfWidth * kArrowAspect;

  dp::UniformValuesStorage uniformStorage;
  uniformStorage.SetFloatValue("u_halfWidth", arrowHalfWidth, glbArrowHalfWidth);

  // calculate arrows
  CalculateArrowBorders(property, kArrowSize, screen.GetScale(), textureWidth, glbArrowHalfWidth);

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
      for (drape_ptr<dp::RenderBucket> const & bucket : property->m_arrow.m_buckets)
        bucket->Render(screen);
    }
  }
}

void RouteRenderer::SetRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng)
{
  m_routeData = move(routeData);

  BuildBuckets(m_routeData->m_route, mng);
  BuildBuckets(m_routeData->m_endOfRouteSign, mng);
  for (drape_ptr<ArrowRenderProperty> const & arrow : m_routeData->m_arrows)
    BuildBuckets(arrow->m_arrow, mng);

  m_distanceFromBegin = 0.0;
}

void RouteRenderer::Clear()
{
  m_routeData.reset();
  m_arrowBorders.clear();
  m_routeSegments.clear();
  m_distanceFromBegin = 0.0;
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::ApplyJoinsBounds(drape_ptr<ArrowRenderProperty> const & property, double joinsBoundsScalar,
                                     double glbHeadLength, vector<ArrowBorders> & arrowBorders)
{
  m_routeSegments.clear();
  m_routeSegments.reserve(2 * property->m_joinsBounds.size() + 1);

  double const length = property->m_end - property->m_start;

  // construct route's segments
  m_routeSegments.emplace_back(0.0, 0.0, true /* m_isAvailable */);
  for (size_t i = 0; i < property->m_joinsBounds.size(); i++)
  {
    double const start = property->m_joinsBounds[i].m_offset +
                         property->m_joinsBounds[i].m_start * joinsBoundsScalar;

    double const end = property->m_joinsBounds[i].m_offset +
                       property->m_joinsBounds[i].m_end * joinsBoundsScalar;

    m_routeSegments.back().m_end = start;
    m_routeSegments.emplace_back(start, end, false /* m_isAvailable */);

    m_routeSegments.emplace_back(end, 0.0, true /* m_isAvailable */);
  }
  m_routeSegments.back().m_end = length;

  // shift head of arrow if necessary
  bool needMerge = false;
  for (size_t i = 0; i < arrowBorders.size(); i++)
  {
    int headIndex = FindNearestAvailableSegment(arrowBorders[i].m_endDistance - glbHeadLength,
                                                arrowBorders[i].m_endDistance, m_routeSegments);
    if (headIndex != SegmentStatus::OK)
    {
      if (headIndex != SegmentStatus::NoSegment)
      {
        ASSERT_GREATER_OR_EQUAL(headIndex, 0, ());
        double const restDist = length - m_routeSegments[headIndex].m_start;
        if (restDist >= glbHeadLength)
          arrowBorders[i].m_endDistance = min(length, m_routeSegments[headIndex].m_start + glbHeadLength);
        else
          arrowBorders[i].m_groupIndex = kInvalidGroup;
      }
      else
      {
        arrowBorders[i].m_groupIndex = kInvalidGroup;
      }
      needMerge = true;
    }
  }

  // merge intersected borders
  if (needMerge)
    MergeAndClipBorders(arrowBorders);
}

void RouteRenderer::CalculateArrowBorders(drape_ptr<ArrowRenderProperty> const & property, double arrowLength,
                                          double scale, double arrowTextureWidth, double joinsBoundsScalar)
{
  ASSERT(!property->m_turns.empty(), ());

  double halfLen = 0.5 * arrowLength;
  double const glbTextureWidth = arrowTextureWidth * scale;
  double const glbTailLength = kArrowTailSize * glbTextureWidth;
  double const glbHeadLength = kArrowHeadSize * glbTextureWidth;

  int const kArrowPartsCount = 3;
  m_arrowBorders.clear();
  m_arrowBorders.reserve(property->m_turns.size() * kArrowPartsCount);

  double const halfTextureWidth = 0.5 * glbTextureWidth;
  if (halfLen < halfTextureWidth)
    halfLen = halfTextureWidth;

  // initial filling
  for (size_t i = 0; i < property->m_turns.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = (int)i;
    arrowBorders.m_startDistance = max(0.0, property->m_turns[i] - halfLen * 0.8);
    arrowBorders.m_endDistance = min(property->m_end - property->m_start, property->m_turns[i] + halfLen * 1.2);

    if (arrowBorders.m_startDistance < m_distanceFromBegin)
      continue;

    m_arrowBorders.push_back(arrowBorders);
  }

  // merge intersected borders and clip them
  MergeAndClipBorders(m_arrowBorders);

  // apply joins bounds to prevent draw arrow's head on a join
  ApplyJoinsBounds(property, joinsBoundsScalar, glbHeadLength, m_arrowBorders);

  // divide to parts of arrow
  size_t const bordersSize = m_arrowBorders.size();
  for (size_t i = 0; i < bordersSize; i++)
  {
    float const startDistance = m_arrowBorders[i].m_startDistance;
    float const endDistance = m_arrowBorders[i].m_endDistance;

    m_arrowBorders[i].m_endDistance = startDistance + glbTailLength;
    m_arrowBorders[i].m_startTexCoord = 0.0;
    m_arrowBorders[i].m_endTexCoord = kArrowTailSize;

    ArrowBorders arrowHead;
    arrowHead.m_startDistance = endDistance - glbHeadLength;
    arrowHead.m_endDistance = endDistance;
    arrowHead.m_startTexCoord = 1.0 - kArrowHeadSize;
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

