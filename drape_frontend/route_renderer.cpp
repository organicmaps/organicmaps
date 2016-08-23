#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"

#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/utils/projection.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace df
{

namespace
{

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
int const kInvalidGroup = -1;

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

bool AreEqualArrowBorders(vector<ArrowBorders> const & borders1, vector<ArrowBorders> const & borders2)
{
  if (borders1.size() != borders2.size())
    return false;

  for (size_t i = 0; i < borders1.size(); i++)
  {
    if (borders1[i].m_groupIndex != borders2[i].m_groupIndex)
      return false;
  }

  double const kDistanceEps = 1e-5;
  for (size_t i = 0; i < borders1.size(); i++)
  {
    if (fabs(borders1[i].m_startDistance - borders2[i].m_startDistance) > kDistanceEps)
      return false;

    if (fabs(borders1[i].m_endDistance - borders2[i].m_endDistance) > kDistanceEps)
      return false;
  }

  return true;
}

} // namespace

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

void RouteRenderer::UpdateRoute(ScreenBase const & screen, TCacheRouteArrowsCallback const & callback)
{
  ASSERT(callback != nullptr, ());

  if (!m_routeData)
    return;

  // Interpolate values by zoom level.
  double zoom = 0.0;
  InterpolateByZoom(screen, m_currentHalfWidth, m_currentAlpha, zoom);

  // Update arrows.
  if (zoom >= kArrowAppearingZoomLevel && !m_routeData->m_sourceTurns.empty())
  {
    // Calculate arrow mercator length.
    double glbHalfLen = 0.5 * kArrowSize;
    double const glbHalfTextureWidth = m_currentHalfWidth * kArrowHeightFactor * screen.GetScale();
    double const glbHalfTextureLen = glbHalfTextureWidth * kArrowAspect;
    if (glbHalfLen < glbHalfTextureLen)
      glbHalfLen = glbHalfTextureLen;

    double const glbArrowHead =  2.0 * kArrowHeadSize * glbHalfTextureLen;
    double const glbArrowTail =  2.0 * kArrowTailSize * glbHalfTextureLen;
    double const glbMinArrowSize = glbArrowHead + glbArrowTail;

    double const kExtendCoef = 1.1;
    m2::RectD screenRect = screen.ClipRect();
    screenRect.Scale(kExtendCoef);

    // Calculate arrow borders.
    vector<ArrowBorders> newArrowBorders;
    newArrowBorders.reserve(m_routeData->m_sourceTurns.size());
    for (size_t i = 0; i < m_routeData->m_sourceTurns.size(); i++)
    {
      ArrowBorders arrowBorders;
      arrowBorders.m_groupIndex = static_cast<int>(i);
      arrowBorders.m_startDistance = max(0.0, m_routeData->m_sourceTurns[i] - glbHalfLen * 0.8);
      arrowBorders.m_endDistance = min(m_routeData->m_length, m_routeData->m_sourceTurns[i] + glbHalfLen * 1.2);

      if ((arrowBorders.m_endDistance - arrowBorders.m_startDistance) < glbMinArrowSize ||
          arrowBorders.m_startDistance < m_distanceFromBegin)
        continue;

      m2::PointD pt = m_routeData->m_sourcePolyline.GetPointByDistance(arrowBorders.m_startDistance);
      if (screenRect.IsPointInside(pt))
      {
        newArrowBorders.push_back(arrowBorders);
        continue;
      }

      pt = m_routeData->m_sourcePolyline.GetPointByDistance(arrowBorders.m_endDistance);
      if (screenRect.IsPointInside(pt))
      {
        newArrowBorders.push_back(arrowBorders);
        continue;
      }
    }

    // Merge intersected borders and clip them.
    MergeAndClipBorders(newArrowBorders);

    // Process head and tail.
    for (ArrowBorders & borders : newArrowBorders)
    {
      borders.m_startDistance += glbArrowTail;
      borders.m_endDistance -= glbArrowHead;
    }

    if (newArrowBorders.empty())
    {
      // Clear arrows.
      m_arrowBorders.clear();
      m_routeArrows.reset();
    }
    else if (!AreEqualArrowBorders(newArrowBorders, m_arrowBorders))
    {
      m_arrowBorders = move(newArrowBorders);
      callback(m_routeData->m_routeIndex, m_arrowBorders);
    }
  }
  else
  {
    m_routeArrows.reset();
  }
}

void RouteRenderer::RenderRoute(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                                dp::UniformValuesStorage const & commonUniforms)
{
  if (!m_routeData)
    return;

  // Render route.
  {
    dp::GLState const & state = m_routeData->m_route.m_state;

    // Set up uniforms.
    dp::UniformValuesStorage uniforms = commonUniforms;
    glsl::vec4 const color = glsl::ToVec4(df::GetColorConstant(GetStyleReader().GetCurrentStyle(),
                                                               m_routeData->m_color));
    uniforms.SetFloatValue("u_color", color.r, color.g, color.b, m_currentAlpha);
    double const screenScale = screen.GetScale();
    uniforms.SetFloatValue("u_routeParams", m_currentHalfWidth, m_currentHalfWidth * screenScale, m_distanceFromBegin);

    if (m_routeData->m_pattern.m_isDashed)
    {
      uniforms.SetFloatValue("u_pattern", m_currentHalfWidth * m_routeData->m_pattern.m_dashLength * screenScale,
                             m_currentHalfWidth * m_routeData->m_pattern.m_gapLength * screenScale);
    }

    // Set up shaders and apply uniforms.
    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(m_routeData->m_pattern.m_isDashed ?
                                                  gpu::ROUTE_DASH_PROGRAM : gpu::ROUTE_PROGRAM);
    prg->Bind();
    dp::ApplyState(state, prg);
    dp::ApplyUniforms(uniforms, prg);

    // Render buckets.
    for (drape_ptr<dp::RenderBucket> const & bucket : m_routeData->m_route.m_buckets)
      bucket->Render();
  }

  // Render arrows.
  if (m_routeArrows != nullptr)
  {
    dp::GLState const & state = m_routeArrows->m_arrows.m_state;

    // set up shaders and apply common uniforms
    dp::UniformValuesStorage uniforms = commonUniforms;
    uniforms.SetFloatValue("u_arrowHalfWidth", m_currentHalfWidth * kArrowHeightFactor);
    uniforms.SetFloatValue("u_opacity", 1.0f);

    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
    prg->Bind();
    dp::ApplyState(state, prg);
    dp::ApplyUniforms(uniforms, prg);
    for (drape_ptr<dp::RenderBucket> const & bucket : m_routeArrows->m_arrows.m_buckets)
      bucket->Render();
  }
}

void RouteRenderer::RenderRouteSigns(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                                     dp::UniformValuesStorage const & commonUniforms)
{
  if (m_startRouteSign)
  {
    ASSERT(m_startRouteSign->m_isValid, ());
    RenderRouteSign(m_startRouteSign, screen, mng, commonUniforms);
  }

  if (m_finishRouteSign)
  {
    ASSERT(m_finishRouteSign->m_isValid, ());
    RenderRouteSign(m_finishRouteSign, screen, mng, commonUniforms);
  }
}

void RouteRenderer::RenderRouteSign(drape_ptr<RouteSignData> const & sign, ScreenBase const & screen,
                                    ref_ptr<dp::GpuProgramManager> mng,
                                    dp::UniformValuesStorage const & commonUniforms)
{
  dp::GLState const & state = sign->m_sign.m_state;

  dp::UniformValuesStorage uniforms = commonUniforms;
  uniforms.SetFloatValue("u_opacity", 1.0f);

  ref_ptr<dp::GpuProgram> program = screen.isPerspective() ? mng->GetProgram(state.GetProgram3dIndex())
                                                           : mng->GetProgram(state.GetProgramIndex());
  program->Bind();

  dp::ApplyState(sign->m_sign.m_state, program);
  dp::ApplyUniforms(uniforms, program);

  for (auto const & bucket : sign->m_sign.m_buckets)
  {
    bucket->GetBuffer()->Build(program);
    bucket->Render();
  }
}

void RouteRenderer::SetRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng)
{
  m_routeData = move(routeData);
  m_arrowBorders.clear();

  BuildBuckets(m_routeData->m_route, mng);
  m_distanceFromBegin = 0.0;
}

void RouteRenderer::SetRouteSign(drape_ptr<RouteSignData> && routeSignData, ref_ptr<dp::GpuProgramManager> mng)
{
  if (routeSignData->m_isStart)
  {
    if (!routeSignData->m_isValid)
    {
      m_startRouteSign.reset();
      return;
    }

    m_startRouteSign = move(routeSignData);
    BuildBuckets(m_startRouteSign->m_sign, mng);
  }
  else
  {
    if (!routeSignData->m_isValid)
    {
      m_finishRouteSign.reset();
      return;
    }

    m_finishRouteSign = move(routeSignData);
    BuildBuckets(m_finishRouteSign->m_sign, mng);
  }
}

drape_ptr<RouteSignData> const & RouteRenderer::GetStartPoint() const
{
  return m_startRouteSign;
}

drape_ptr<RouteSignData> const & RouteRenderer::GetFinishPoint() const
{
  return m_finishRouteSign;
}

drape_ptr<RouteData> const & RouteRenderer::GetRouteData() const
{
  return m_routeData;
}

void RouteRenderer::SetRouteArrows(drape_ptr<RouteArrowsData> && routeArrowsData,
                                   ref_ptr<dp::GpuProgramManager> mng)
{
  m_routeArrows = move(routeArrowsData);
  BuildBuckets(m_routeArrows->m_arrows, mng);
}

void RouteRenderer::Clear(bool keepDistanceFromBegin)
{
  m_routeData.reset();
  m_startRouteSign.reset();
  m_finishRouteSign.reset();
  m_arrowBorders.clear();
  m_routeArrows.reset();

  if (!keepDistanceFromBegin)
    m_distanceFromBegin = 0.0;
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

} // namespace df
