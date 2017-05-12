#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/glsl_func.hpp"
#include "drape/utils/projection.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"

namespace df
{

std::string const kRouteColor = "Route";
std::string const kRouteOutlineColor = "RouteOutline";
std::string const kRoutePedestrian = "RoutePedestrian";
std::string const kRouteBicycle = "RouteBicycle";

namespace
{

float const kHalfWidthInPixelVehicle[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.5f, 1.5f, 1.5f, 2.0f, 2.0f, 2.0f, 2.5f, 2.5f,
  //11   12    13    14    15   16    17    18    19     20
  3.0f, 3.0f, 4.0f, 5.0f, 6.0, 8.0f, 10.0f, 10.0f, 18.0f, 27.0f
};

float const kHalfWidthInPixelOthers[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f,
  //11   12    13    14    15   16    17    18    19     20
  1.5f, 1.5f, 2.0f, 2.5f, 3.0, 4.0f, 5.0f, 5.0f, 9.0f, 13.0f
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

dp::Color GetOutlineColor(df::ColorConstant const & routeColor)
{
  return df::GetColorConstant(routeColor == kRouteColor ? kRouteOutlineColor : routeColor);
}

} // namespace

RouteRenderer::RouteRenderer()
  : m_distanceFromBegin(0.0)
{}

void RouteRenderer::InterpolateByZoom(ScreenBase const & screen, ColorConstant color,
                                      float & halfWidth, double & zoom) const
{
  double const zoomLevel = GetZoomLevel(screen.GetScale());
  zoom = trunc(zoomLevel);
  int const index = zoom - 1.0;
  float const lerpCoef = zoomLevel - zoom;

  float const * halfWidthInPixel = kHalfWidthInPixelVehicle;
  if (color != kRouteColor)
    halfWidthInPixel = kHalfWidthInPixelOthers;

  if (index < scales::UPPER_STYLE_SCALE)
    halfWidth = halfWidthInPixel[index] + lerpCoef * (halfWidthInPixel[index + 1] - halfWidthInPixel[index]);
  else
    halfWidth = halfWidthInPixel[scales::UPPER_STYLE_SCALE];

  halfWidth *= df::VisualParams::Instance().GetVisualScale();
}

void RouteRenderer::UpdateRoute(ScreenBase const & screen, TCacheRouteArrowsCallback const & callback)
{
  ASSERT(callback != nullptr, ());

  if (!m_routeData)
    return;

  // Interpolate values by zoom level.
  double zoom = 0.0;
  InterpolateByZoom(screen, m_routeData->m_color, m_currentHalfWidth, zoom);

  // Update arrows.
  if (zoom >= kArrowAppearingZoomLevel && !m_routeData->m_sourceTurns.empty())
  {
    // Calculate arrow mercator length.
    double glbHalfLen = 0.5 * kArrowSize;
    double const glbHalfTextureWidth = m_currentHalfWidth * kArrowHeightFactor * screen.GetScale();
    double const glbHalfTextureLen = glbHalfTextureWidth * kArrowAspect;
    if (glbHalfLen < glbHalfTextureLen)
      glbHalfLen = glbHalfTextureLen;

    double const glbArrowHead = 2.0 * kArrowHeadSize * glbHalfTextureLen;
    double const glbArrowTail = 2.0 * kArrowTailSize * glbHalfTextureLen;
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

void RouteRenderer::RenderRoute(ScreenBase const & screen, bool trafficShown,
                                ref_ptr<dp::GpuProgramManager> mng,
                                dp::UniformValuesStorage const & commonUniforms)
{
  if (!m_routeData || m_routeData->m_route.m_buckets.empty())
    return;

  // Render route.
  {
    dp::GLState const & state = m_routeData->m_route.m_state;

    // Set up uniforms.
    dp::UniformValuesStorage uniforms = commonUniforms;
    math::Matrix<float, 4, 4> mv = screen.GetModelView(m_routeData->m_pivot, kShapeCoordScalar);
    uniforms.SetMatrix4x4Value("modelView", mv.m_data);
    glsl::vec4 const color = glsl::ToVec4(df::GetColorConstant(m_routeData->m_color));
    uniforms.SetFloatValue("u_color", color.r, color.g, color.b, color.a);
    double const screenScale = screen.GetScale();
    uniforms.SetFloatValue("u_routeParams", m_currentHalfWidth, m_currentHalfWidth * screenScale,
                           m_distanceFromBegin, trafficShown ? 1.0f : 0.0f);

    if (m_routeData->m_pattern.m_isDashed)
    {
      uniforms.SetFloatValue("u_pattern", m_currentHalfWidth * m_routeData->m_pattern.m_dashLength * screenScale,
                             m_currentHalfWidth * m_routeData->m_pattern.m_gapLength * screenScale);
    }
    else
    {
      glsl::vec4 const outlineColor = glsl::ToVec4(GetOutlineColor(m_routeData->m_color));
      uniforms.SetFloatValue("u_outlineColor", outlineColor.r, outlineColor.g, outlineColor.b, outlineColor.a);
    }

    // Set up shaders and apply uniforms.
    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(m_routeData->m_pattern.m_isDashed ?
                                                  gpu::ROUTE_DASH_PROGRAM : gpu::ROUTE_PROGRAM);
    prg->Bind();
    dp::ApplyState(state, prg);
    dp::ApplyUniforms(uniforms, prg);

    // Render buckets.
    for (drape_ptr<dp::RenderBucket> const & bucket : m_routeData->m_route.m_buckets)
      bucket->Render(state.GetDrawAsLine());
  }

  // Render arrows.
  if (m_routeArrows != nullptr)
  {
    dp::GLState const & state = m_routeArrows->m_arrows.m_state;

    // set up shaders and apply common uniforms
    dp::UniformValuesStorage uniforms = commonUniforms;
    math::Matrix<float, 4, 4> mv = screen.GetModelView(m_routeArrows->m_pivot, kShapeCoordScalar);
    uniforms.SetMatrix4x4Value("modelView", mv.m_data);
    uniforms.SetFloatValue("u_arrowHalfWidth", m_currentHalfWidth * kArrowHeightFactor);
    uniforms.SetFloatValue("u_opacity", 1.0f);

    ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
    prg->Bind();
    dp::ApplyState(state, prg);
    dp::ApplyUniforms(uniforms, prg);
    for (drape_ptr<dp::RenderBucket> const & bucket : m_routeArrows->m_arrows.m_buckets)
      bucket->Render(state.GetDrawAsLine());
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
  if (sign->m_sign.m_buckets.empty())
    return;

  dp::GLState const & state = sign->m_sign.m_state;
  dp::UniformValuesStorage uniforms = commonUniforms;
  math::Matrix<float, 4, 4> mv = screen.GetModelView(sign->m_position, 1.0);
  uniforms.SetMatrix4x4Value("modelView", mv.m_data);
  uniforms.SetFloatValue("u_opacity", 1.0f);

  ref_ptr<dp::GpuProgram> program = screen.isPerspective() ? mng->GetProgram(state.GetProgram3dIndex())
                                                           : mng->GetProgram(state.GetProgramIndex());
  program->Bind();

  dp::ApplyState(state, program);
  dp::ApplyUniforms(uniforms, program);

  for (auto const & bucket : sign->m_sign.m_buckets)
  {
    bucket->GetBuffer()->Build(program);
    bucket->Render(state.GetDrawAsLine());
  }
}

void RouteRenderer::SetRouteData(drape_ptr<RouteData> && routeData, ref_ptr<dp::GpuProgramManager> mng)
{
  m_routeData = move(routeData);
  m_arrowBorders.clear();

  BuildBuckets(m_routeData->m_route, mng);
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

void RouteRenderer::Clear()
{
  m_routeData.reset();
  m_startRouteSign.reset();
  m_finishRouteSign.reset();
  m_arrowBorders.clear();
  m_routeArrows.reset();
  m_distanceFromBegin = 0.0;
}

void RouteRenderer::ClearGLDependentResources()
{
  if (m_routeData != nullptr)
    m_routeData->m_route = RouteRenderProperty();
  if (m_startRouteSign != nullptr)
    m_startRouteSign->m_sign = RouteRenderProperty();
  if (m_finishRouteSign != nullptr)
    m_finishRouteSign->m_sign = RouteRenderProperty();
  m_routeArrows.reset();
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

} // namespace df
