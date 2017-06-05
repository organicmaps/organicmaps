#include "drape_frontend/route_renderer.hpp"
#include "drape_frontend/message_subclasses.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/shape_view_params.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/glsl_func.hpp"
#include "drape/utils/projection.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
std::string const kRouteColor = "Route";
std::string const kRouteOutlineColor = "RouteOutline";
std::string const kRoutePedestrian = "RoutePedestrian";
std::string const kRouteBicycle = "RouteBicycle";

namespace
{
float const kHalfWidthInPixelCar[] =
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

void ClipBorders(std::vector<ArrowBorders> & borders)
{
  auto invalidBorders = [](ArrowBorders const & borders)
  {
    return borders.m_groupIndex == kInvalidGroup;
  };
  borders.erase(std::remove_if(borders.begin(), borders.end(), invalidBorders), borders.end());
}

void MergeAndClipBorders(std::vector<ArrowBorders> & borders)
{
  // Initial clipping.
  ClipBorders(borders);

  if (borders.empty())
    return;

  // Mark groups.
  for (size_t i = 0; i + 1 < borders.size(); i++)
  {
    if (borders[i].m_endDistance >= borders[i + 1].m_startDistance)
      borders[i + 1].m_groupIndex = borders[i].m_groupIndex;
  }

  // Merge groups.
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

  // Clip groups.
  ClipBorders(borders);
}

bool AreEqualArrowBorders(std::vector<ArrowBorders> const & borders1,
                          std::vector<ArrowBorders> const & borders2)
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

std::vector<ArrowBorders> CalculateArrowBorders(ScreenBase const & screen, float currentHalfWidth,
                                                drape_ptr<RouteData> const & routeData,
                                                double distanceFromBegin)
{
  if (routeData->m_segment->m_turns.empty())
    return {};

  // Calculate arrow mercator length.
  double glbHalfLen = 0.5 * kArrowSize;
  double const glbHalfTextureWidth = currentHalfWidth * kArrowHeightFactor * screen.GetScale();
  double const glbHalfTextureLen = glbHalfTextureWidth * kArrowAspect;
  if (glbHalfLen < glbHalfTextureLen)
    glbHalfLen = glbHalfTextureLen;

  double const glbArrowHead = 2.0 * kArrowHeadSize * glbHalfTextureLen;
  double const glbArrowTail = 2.0 * kArrowTailSize * glbHalfTextureLen;
  double const glbMinArrowSize = glbArrowHead + glbArrowTail;

  double const kExtendCoef = 1.1;
  m2::RectD screenRect = screen.ClipRect();
  screenRect.Scale(kExtendCoef);

  auto const & segment = routeData->m_segment;

  // Calculate arrow borders.
  std::vector<ArrowBorders> newArrowBorders;
  newArrowBorders.reserve(segment->m_turns.size());
  for (size_t i = 0; i < segment->m_turns.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = static_cast<int>(i);
    arrowBorders.m_startDistance = std::max(0.0, segment->m_turns[i] - glbHalfLen * 0.8);
    arrowBorders.m_endDistance = std::min(routeData->m_length, segment->m_turns[i] + glbHalfLen * 1.2);

    if ((arrowBorders.m_endDistance - arrowBorders.m_startDistance) < glbMinArrowSize ||
        arrowBorders.m_startDistance < distanceFromBegin)
    {
      continue;
    }

    m2::PointD pt = segment->m_polyline.GetPointByDistance(arrowBorders.m_startDistance);
    if (screenRect.IsPointInside(pt))
    {
      newArrowBorders.push_back(arrowBorders);
      continue;
    }

    pt = segment->m_polyline.GetPointByDistance(arrowBorders.m_endDistance);
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

  return newArrowBorders;
}

void BuildBuckets(RouteRenderProperty const & renderProperty, ref_ptr<dp::GpuProgramManager> mng)
{
  for (auto const & bucket : renderProperty.m_buckets)
    bucket->GetBuffer()->Build(mng->GetProgram(renderProperty.m_state.GetProgramIndex()));
}

dp::Color GetOutlineColor(drape_ptr<RouteSegment> const & routeSegment)
{
  if (routeSegment->m_routeType == RouteType::Car || routeSegment->m_routeType == RouteType::Taxi)
    return df::GetColorConstant(kRouteOutlineColor);

  return df::GetColorConstant(routeSegment->m_color);
}
}  // namespace

RouteRenderer::RouteRenderer()
  : m_distanceFromBegin(0.0)
  , m_followingEnabled(false)
{}

void RouteRenderer::InterpolateByZoom(drape_ptr<RouteSegment> const & routeSegment, ScreenBase const & screen,
                                      float & halfWidth, double & zoom) const
{
  double const zoomLevel = GetZoomLevel(screen.GetScale());
  zoom = trunc(zoomLevel);
  int const index = static_cast<int>(zoom - 1.0);
  float const lerpCoef = static_cast<float>(zoomLevel - zoom);

  float const * halfWidthInPixel = kHalfWidthInPixelOthers;
  if (routeSegment->m_routeType == RouteType::Car || routeSegment->m_routeType == RouteType::Taxi)
    halfWidthInPixel = kHalfWidthInPixelCar;

  if (index < scales::UPPER_STYLE_SCALE)
    halfWidth = halfWidthInPixel[index] + lerpCoef * (halfWidthInPixel[index + 1] - halfWidthInPixel[index]);
  else
    halfWidth = halfWidthInPixel[scales::UPPER_STYLE_SCALE];

  halfWidth *= df::VisualParams::Instance().GetVisualScale();
}

void RouteRenderer::UpdateRoute(ScreenBase const & screen, TCacheRouteArrowsCallback const & callback)
{
  ASSERT(callback != nullptr, ());
  for (auto const & routeData : m_routeData)
  {
    auto & additional = m_routeAdditional[routeData->m_segmentId];

    // Interpolate values by zoom level.
    double zoom = 0.0;
    float halfWidth = 0.0;
    InterpolateByZoom(routeData->m_segment, screen, halfWidth, zoom);
    additional.m_currentHalfWidth = halfWidth;

    if (zoom < kArrowAppearingZoomLevel)
    {
      additional.m_arrowsData.reset();
      additional.m_arrowBorders.clear();
      continue;
    }

    // Calculate arrow borders.
    auto newArrowBorders = CalculateArrowBorders(screen, halfWidth, routeData,
                                                 m_followingEnabled ? m_distanceFromBegin : -1.0f);
    if (newArrowBorders.empty())
    {
      // Clear arrows.
      additional.m_arrowsData.reset();
      additional.m_arrowBorders.clear();
    }
    else if (!AreEqualArrowBorders(newArrowBorders, additional.m_arrowBorders))
    {
      additional.m_arrowBorders = std::move(newArrowBorders);
      callback(routeData->m_segmentId, additional.m_arrowBorders);
    }
  }
}

void RouteRenderer::RenderRouteData(drape_ptr<RouteData> const & routeData,
                                    ScreenBase const & screen, bool trafficShown,
                                    ref_ptr<dp::GpuProgramManager> mng,
                                    dp::UniformValuesStorage const & commonUniforms)
{
  if (routeData->m_renderProperty.m_buckets.empty())
    return;

  float const currentHalfWidth = m_routeAdditional[routeData->m_segmentId].m_currentHalfWidth;
  float const screenHalfWidth = static_cast<float>(currentHalfWidth * screen.GetScale());
  float const dist = m_followingEnabled ? static_cast<float>(m_distanceFromBegin) : -1.0f;

  dp::GLState const & state = routeData->m_renderProperty.m_state;
  auto const & segment = routeData->m_segment;

  // Set up uniforms.
  dp::UniformValuesStorage uniforms = commonUniforms;
  math::Matrix<float, 4, 4> mv = screen.GetModelView(routeData->m_pivot, kShapeCoordScalar);
  uniforms.SetMatrix4x4Value("modelView", mv.m_data);

  glsl::vec4 const color = glsl::ToVec4(df::GetColorConstant(segment->m_color));
  uniforms.SetFloatValue("u_color", color.r, color.g, color.b, color.a);

  uniforms.SetFloatValue("u_routeParams", currentHalfWidth, screenHalfWidth, dist,
                         trafficShown ? 1.0f : 0.0f);

  if (segment->m_pattern.m_isDashed)
  {
    uniforms.SetFloatValue("u_pattern",
                           static_cast<float>(screenHalfWidth * segment->m_pattern.m_dashLength),
                           static_cast<float>(screenHalfWidth * segment->m_pattern.m_gapLength));
  }
  else
  {
    glsl::vec4 const outlineColor = glsl::ToVec4(GetOutlineColor(segment));
    uniforms.SetFloatValue("u_outlineColor", outlineColor.r, outlineColor.g,
                           outlineColor.b, outlineColor.a);
  }

  // Set up shaders and apply uniforms.
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(segment->m_pattern.m_isDashed ?
                                                gpu::ROUTE_DASH_PROGRAM : gpu::ROUTE_PROGRAM);
  prg->Bind();
  dp::ApplyState(state, prg);
  dp::ApplyUniforms(uniforms, prg);

  // Render buckets.
  for (auto const & bucket : routeData->m_renderProperty.m_buckets)
    bucket->Render(state.GetDrawAsLine());
}

void RouteRenderer::RenderRouteArrowData(dp::DrapeID segmentId, RouteAdditional const & routeAdditional,
                                         ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                                         dp::UniformValuesStorage const & commonUniforms)
{
  if (routeAdditional.m_arrowsData == nullptr ||
      routeAdditional.m_arrowsData->m_renderProperty.m_buckets.empty())
  {
    return;
  }

  float const currentHalfWidth = m_routeAdditional[segmentId].m_currentHalfWidth;
  dp::GLState const & state = routeAdditional.m_arrowsData->m_renderProperty.m_state;

  // Set up shaders and apply common uniforms.
  dp::UniformValuesStorage uniforms = commonUniforms;
  math::Matrix<float, 4, 4> mv = screen.GetModelView(routeAdditional.m_arrowsData->m_pivot,
                                                     kShapeCoordScalar);
  uniforms.SetMatrix4x4Value("modelView", mv.m_data);
  uniforms.SetFloatValue("u_arrowHalfWidth", static_cast<float>(currentHalfWidth * kArrowHeightFactor));
  uniforms.SetFloatValue("u_opacity", 1.0f);

  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(gpu::ROUTE_ARROW_PROGRAM);
  prg->Bind();
  dp::ApplyState(state, prg);
  dp::ApplyUniforms(uniforms, prg);
  for (auto const & bucket : routeAdditional.m_arrowsData->m_renderProperty.m_buckets)
    bucket->Render(state.GetDrawAsLine());
}

void RouteRenderer::RenderRoute(ScreenBase const & screen, bool trafficShown,
                                ref_ptr<dp::GpuProgramManager> mng,
                                dp::UniformValuesStorage const & commonUniforms)
{
  if (m_routeData.empty())
    return;

  // Render route.
  for (auto const & routeData : m_routeData)
    RenderRouteData(routeData, screen, trafficShown, mng, commonUniforms);

  // Render arrows.
  for (auto const & p : m_routeAdditional)
    RenderRouteArrowData(p.first, p.second, screen, mng, commonUniforms);
}

void RouteRenderer::AddRouteData(drape_ptr<RouteData> && routeData,
                                 ref_ptr<dp::GpuProgramManager> mng)
{
  // Remove old route data with the same id.
  RemoveRouteData(routeData->m_segmentId);

  // Add new route data.
  m_routeData.push_back(std::move(routeData));
  BuildBuckets(m_routeData.back()->m_renderProperty, mng);
}

std::vector<drape_ptr<RouteData>> const & RouteRenderer::GetRouteData() const
{
  return m_routeData;
}

void RouteRenderer::RemoveRouteData(dp::DrapeID segmentId)
{
  auto const functor = [&segmentId](drape_ptr<RouteData> const & data)
  {
    return data->m_segmentId == segmentId;
  };
  m_routeData.erase(std::remove_if(m_routeData.begin(), m_routeData.end(), functor),
                    m_routeData.end());
  m_routeAdditional[segmentId].m_arrowsData.reset();
  m_routeAdditional[segmentId].m_arrowBorders.clear();
}

void RouteRenderer::AddRouteArrowsData(drape_ptr<RouteArrowsData> && routeArrowsData,
                                       ref_ptr<dp::GpuProgramManager> mng)
{
  auto & additional = m_routeAdditional[routeArrowsData->m_segmentId];
  additional.m_arrowsData = std::move(routeArrowsData);
  BuildBuckets(additional.m_arrowsData->m_renderProperty, mng);
}

void RouteRenderer::ClearRouteData()
{
  m_routeData.clear();
  m_routeAdditional.clear();
}

void RouteRenderer::Clear()
{
  ClearRouteData();
  m_distanceFromBegin = 0.0;
}

void RouteRenderer::ClearGLDependentResources()
{
  // Here we clear only GL-dependent part of route data.
  for (auto & routeData : m_routeData)
    routeData->m_renderProperty = RouteRenderProperty();

  // All additional data (like arrows) will be regenerated, so clear them all.
  m_routeAdditional.clear();
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::SetFollowingEnabled(bool enabled)
{
  m_followingEnabled = enabled;
}
}  // namespace df
