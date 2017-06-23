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
#include <cmath>

namespace df
{
std::string const kRouteColor = "Route";
std::string const kRouteOutlineColor = "RouteOutline";
std::string const kRoutePedestrian = "RoutePedestrian";
std::string const kRouteBicycle = "RouteBicycle";
std::string const kRoutePreview = "RoutePreview";

namespace
{
std::vector<float> const kHalfWidthInPixelCar =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.5f, 1.5f, 1.5f, 2.0f, 2.0f, 2.0f, 2.5f, 2.5f,
  //11   12    13    14    15   16    17    18    19     20
  3.0f, 3.0f, 4.0f, 5.0f, 6.0, 8.0f, 10.0f, 10.0f, 18.0f, 27.0f
};

std::vector<float> const kHalfWidthInPixelOthers =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.2f, 1.2f,
  //11   12    13    14    15   16    17    18    19     20
  1.5f, 1.5f, 2.0f, 2.5f, 3.0, 4.0f, 5.0f, 5.0f, 9.0f, 13.0f
};

std::vector<float> const kPreviewPointRadiusInPixel =
{
  // 1   2     3     4     5     6     7     8     9     10
  0.8f, 0.8f, 0.8f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f,
  //11   12    13    14    15    16    17    18    19     20
  2.5f, 2.5f, 2.5f, 2.5f, 2.5f, 3.5f, 4.5f, 4.5f, 4.5f, 5.5f
};

int const kArrowAppearingZoomLevel = 14;
int const kInvalidGroup = -1;

uint32_t const kPreviewPointsCount = 512;
double const kPreviewAnimationSpeed = 3.0;
double const kPreviewAnimationScale = 0.3;

double const kInvalidDistance = -1.0;

void InterpolateByZoom(drape_ptr<Subroute> const & subroute, ScreenBase const & screen,
                       float & halfWidth, double & zoom)
{
  int index = 0;
  float lerpCoef = 0.0f;
  ExtractZoomFactors(screen, zoom, index, lerpCoef);

  std::vector<float> const * halfWidthInPixel = &kHalfWidthInPixelOthers;
  if (subroute->m_routeType == RouteType::Car || subroute->m_routeType == RouteType::Taxi)
    halfWidthInPixel = &kHalfWidthInPixelCar;

  halfWidth = InterpolateByZoomLevels(index, lerpCoef, *halfWidthInPixel);
  halfWidth *= static_cast<float>(df::VisualParams::Instance().GetVisualScale());
}

float CalculateRadius(ScreenBase const & screen)
{
  double zoom = 0.0;
  int index = 0;
  float lerpCoef = 0.0f;
  ExtractZoomFactors(screen, zoom, index, lerpCoef);
  float const radius = InterpolateByZoomLevels(index, lerpCoef, kPreviewPointRadiusInPixel);
  return radius * static_cast<float>(VisualParams::Instance().GetVisualScale());
}

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
  if (routeData->m_subroute->m_turns.empty())
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

  auto const & subroute = routeData->m_subroute;

  // Calculate arrow borders.
  std::vector<ArrowBorders> newArrowBorders;
  newArrowBorders.reserve(subroute->m_turns.size());
  for (size_t i = 0; i < subroute->m_turns.size(); i++)
  {
    ArrowBorders arrowBorders;
    arrowBorders.m_groupIndex = static_cast<int>(i);
    arrowBorders.m_startDistance = std::max(0.0, subroute->m_turns[i] - glbHalfLen * 0.8);
    arrowBorders.m_endDistance = std::min(routeData->m_length, subroute->m_turns[i] + glbHalfLen * 1.2);

    if ((arrowBorders.m_endDistance - arrowBorders.m_startDistance) < glbMinArrowSize ||
        arrowBorders.m_startDistance < distanceFromBegin)
    {
      continue;
    }

    m2::PointD pt = subroute->m_polyline.GetPointByDistance(arrowBorders.m_startDistance);
    if (screenRect.IsPointInside(pt))
    {
      newArrowBorders.push_back(arrowBorders);
      continue;
    }

    pt = subroute->m_polyline.GetPointByDistance(arrowBorders.m_endDistance);
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

dp::Color GetOutlineColor(drape_ptr<Subroute> const & subroute)
{
  if (subroute->m_routeType == RouteType::Car || subroute->m_routeType == RouteType::Taxi)
    return df::GetColorConstant(kRouteOutlineColor);

  return df::GetColorConstant(subroute->m_color);
}
}  // namespace

RouteRenderer::RouteRenderer(PreviewPointsRequestCallback && previewPointsRequest)
  : m_distanceFromBegin(kInvalidDistance)
  , m_followingEnabled(false)
  , m_previewPointsRequest(std::move(previewPointsRequest))
  , m_waitForPreviewRenderData(false)
{
  ASSERT(m_previewPointsRequest != nullptr, ());
}

void RouteRenderer::UpdateRoute(ScreenBase const & screen, CacheRouteArrowsCallback const & callback)
{
  ASSERT(callback != nullptr, ());
  for (auto const & routeData : m_routeData)
  {
    auto & additional = m_routeAdditional[routeData->m_subrouteId];

    // Interpolate values by zoom level.
    double zoom = 0.0;
    float halfWidth = 0.0;
    InterpolateByZoom(routeData->m_subroute, screen, halfWidth, zoom);
    additional.m_currentHalfWidth = halfWidth;

    if (zoom < kArrowAppearingZoomLevel)
    {
      additional.m_arrowsData.reset();
      additional.m_arrowBorders.clear();
      continue;
    }

    // Calculate arrow borders.
    double dist = kInvalidDistance;
    if (m_followingEnabled)
      dist = m_distanceFromBegin - routeData->m_subroute->m_baseDistance;
    auto newArrowBorders = CalculateArrowBorders(screen, halfWidth, routeData, dist);
    if (newArrowBorders.empty())
    {
      // Clear arrows.
      additional.m_arrowsData.reset();
      additional.m_arrowBorders.clear();
    }
    else if (!AreEqualArrowBorders(newArrowBorders, additional.m_arrowBorders))
    {
      additional.m_arrowBorders = std::move(newArrowBorders);
      callback(routeData->m_subrouteId, additional.m_arrowBorders);
    }
  }
}

bool RouteRenderer::UpdatePreview(ScreenBase const & screen)
{
  // Check if there are preview render data.
  if (m_previewRenderData.empty() && !m_waitForPreviewRenderData)
  {
    m_previewPointsRequest(kPreviewPointsCount);
    m_waitForPreviewRenderData = true;
  }
  if (m_waitForPreviewRenderData)
    return false;

  float scale = 1.0f;
  float previewCircleRadius = 0.0;
  if (!m_previewSegments.empty())
  {
    ClearPreviewHandles();
    m_previewPivot = screen.GlobalRect().Center();
    previewCircleRadius = CalculateRadius(screen);

    using namespace std::chrono;
    auto dt = steady_clock::now() - m_showPreviewTimestamp;
    double const seconds = duration_cast<duration<double>>(dt).count();
    scale = static_cast<float>(1.0 + kPreviewAnimationScale *
                                     abs(sin(kPreviewAnimationSpeed * seconds)));
  }
  double const currentScaleGtoP = 1.0 / screen.GetScale();
  double const radiusMercator = previewCircleRadius / currentScaleGtoP;
  double const diameterMercator = 2.0 * radiusMercator;
  double const gapMercator = diameterMercator;
  dp::Color const circleColor = df::GetColorConstant(kRoutePreview);

  for (auto const & previewSegment : m_previewSegments)
  {
    auto const & info = previewSegment.second;
    m2::PolylineD polyline = {info.m_startPoint, info.m_finishPoint};
    double const segmentLen = polyline.GetLength();
    size_t circlesCount = static_cast<size_t>(segmentLen / (diameterMercator + gapMercator));
    if (circlesCount == 0)
      circlesCount = 1;
    double const distDelta = segmentLen / circlesCount;
    for (double d = distDelta * 0.5; d < segmentLen; d += distDelta)
    {
      float const r = scale * static_cast<float>(radiusMercator);
      m2::PointD const pt = polyline.GetPointByDistance(d);
      m2::RectD const circleRect(pt.x - r, pt.y - r, pt.x + r, pt.y + r);
      if (!screen.ClipRect().IsIntersect(circleRect))
        continue;

      size_t pointIndex;
      CirclesPackHandle * h = GetPreviewHandle(pointIndex);
      if (h == nullptr)
      {
        // There is no any available handle.
        m_previewPointsRequest(kPreviewPointsCount);
        m_waitForPreviewRenderData = true;
        return true;
      }

      m2::PointD const convertedPt = MapShape::ConvertToLocal(pt, m_previewPivot, kShapeCoordScalar);
      h->SetPoint(pointIndex, convertedPt, scale * previewCircleRadius, circleColor);
    }
  }
  return !m_previewSegments.empty();
}

void RouteRenderer::ClearPreviewHandles()
{
  for (auto & handle : m_previewHandlesCache)
  {
    handle.first->Clear();
    handle.second = 0;
  }
}

CirclesPackHandle * RouteRenderer::GetPreviewHandle(size_t & index)
{
  for (auto & handle : m_previewHandlesCache)
  {
    if (handle.second < handle.first->GetPointsCount())
    {
      index = handle.second++;
      return handle.first;
    }
  }
  index = 0;
  return nullptr;
}

void RouteRenderer::RenderRouteData(drape_ptr<RouteData> const & routeData,
                                    ScreenBase const & screen, bool trafficShown,
                                    ref_ptr<dp::GpuProgramManager> mng,
                                    dp::UniformValuesStorage const & commonUniforms)
{
  if (routeData->m_renderProperty.m_buckets.empty())
    return;

  // Skip rendering of hidden subroutes.
  if (m_hiddenSubroutes.find(routeData->m_subrouteId) != m_hiddenSubroutes.end())
    return;

  float const currentHalfWidth = m_routeAdditional[routeData->m_subrouteId].m_currentHalfWidth;
  float const screenHalfWidth = static_cast<float>(currentHalfWidth * screen.GetScale());
  float dist = static_cast<float>(kInvalidDistance);
  if (m_followingEnabled)
    dist = static_cast<float>(m_distanceFromBegin - routeData->m_subroute->m_baseDistance);

  dp::GLState const & state = routeData->m_renderProperty.m_state;
  auto const & subroute = routeData->m_subroute;

  // Set up uniforms.
  dp::UniformValuesStorage uniforms = commonUniforms;
  math::Matrix<float, 4, 4> mv = screen.GetModelView(routeData->m_pivot, kShapeCoordScalar);
  uniforms.SetMatrix4x4Value("modelView", mv.m_data);

  glsl::vec4 const color = glsl::ToVec4(df::GetColorConstant(subroute->m_color));
  uniforms.SetFloatValue("u_color", color.r, color.g, color.b, color.a);

  uniforms.SetFloatValue("u_routeParams", currentHalfWidth, screenHalfWidth, dist,
                         trafficShown ? 1.0f : 0.0f);

  if (subroute->m_pattern.m_isDashed)
  {
    uniforms.SetFloatValue("u_pattern",
                           static_cast<float>(screenHalfWidth * subroute->m_pattern.m_dashLength),
                           static_cast<float>(screenHalfWidth * subroute->m_pattern.m_gapLength));
  }
  else
  {
    glsl::vec4 const outlineColor = glsl::ToVec4(GetOutlineColor(subroute));
    uniforms.SetFloatValue("u_outlineColor", outlineColor.r, outlineColor.g,
                           outlineColor.b, outlineColor.a);
  }

  // Set up shaders and apply uniforms.
  ref_ptr<dp::GpuProgram> prg = mng->GetProgram(subroute->m_pattern.m_isDashed ?
                                                gpu::ROUTE_DASH_PROGRAM : gpu::ROUTE_PROGRAM);
  prg->Bind();
  dp::ApplyState(state, prg);
  dp::ApplyUniforms(uniforms, prg);

  // Render buckets.
  for (auto const & bucket : routeData->m_renderProperty.m_buckets)
    bucket->Render(state.GetDrawAsLine());
}

void RouteRenderer::RenderRouteArrowData(dp::DrapeID subrouteId, RouteAdditional const & routeAdditional,
                                         ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                                         dp::UniformValuesStorage const & commonUniforms)
{
  if (routeAdditional.m_arrowsData == nullptr ||
      routeAdditional.m_arrowsData->m_renderProperty.m_buckets.empty())
  {
    return;
  }

  float const currentHalfWidth = m_routeAdditional[subrouteId].m_currentHalfWidth;
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

void RouteRenderer::RenderPreviewData(ScreenBase const & screen, ref_ptr<dp::GpuProgramManager> mng,
                                      dp::UniformValuesStorage const & commonUniforms)
{
  if (m_waitForPreviewRenderData || m_previewSegments.empty() || m_previewRenderData.empty())
    return;

  dp::UniformValuesStorage uniforms = commonUniforms;
  math::Matrix<float, 4, 4> mv = screen.GetModelView(m_previewPivot, kShapeCoordScalar);
  uniforms.SetMatrix4x4Value("modelView", mv.m_data);
  uniforms.SetFloatValue("u_opacity", 1.0f);
  ref_ptr<dp::GpuProgram> program = mng->GetProgram(gpu::CIRCLE_POINT_PROGRAM);
  program->Bind();

  dp::GLState const & state = m_previewRenderData.front()->m_state;
  dp::ApplyState(state, program);
  dp::ApplyUniforms(uniforms, program);

  ASSERT_EQUAL(m_previewRenderData.size(), m_previewHandlesCache.size(), ());
  for (size_t i = 0; i < m_previewRenderData.size(); i++)
  {
    if (m_previewHandlesCache[i].second != 0)
      m_previewRenderData[i]->m_bucket->Render(state.GetDrawAsLine());
  }
}

void RouteRenderer::RenderRoute(ScreenBase const & screen, bool trafficShown,
                                ref_ptr<dp::GpuProgramManager> mng,
                                dp::UniformValuesStorage const & commonUniforms)
{
  if (!m_routeData.empty())
  {
    // Render route.
    for (auto const & routeData : m_routeData)
      RenderRouteData(routeData, screen, trafficShown, mng, commonUniforms);

    // Render arrows.
    for (auto const & p : m_routeAdditional)
      RenderRouteArrowData(p.first, p.second, screen, mng, commonUniforms);
  }

  // Render preview.
  RenderPreviewData(screen, mng, commonUniforms);
}

void RouteRenderer::AddRouteData(drape_ptr<RouteData> && routeData,
                                 ref_ptr<dp::GpuProgramManager> mng)
{
  // Remove old route data with the same id.
  RemoveRouteData(routeData->m_subrouteId);

  // Add new route data.
  m_routeData.push_back(std::move(routeData));
  BuildBuckets(m_routeData.back()->m_renderProperty, mng);
}

std::vector<drape_ptr<RouteData>> const & RouteRenderer::GetRouteData() const
{
  return m_routeData;
}

void RouteRenderer::RemoveRouteData(dp::DrapeID subrouteId)
{
  auto const functor = [&subrouteId](drape_ptr<RouteData> const & data)
  {
    return data->m_subrouteId == subrouteId;
  };
  m_routeData.erase(std::remove_if(m_routeData.begin(), m_routeData.end(), functor),
                    m_routeData.end());
  m_routeAdditional[subrouteId].m_arrowsData.reset();
  m_routeAdditional[subrouteId].m_arrowBorders.clear();
}

void RouteRenderer::AddRouteArrowsData(drape_ptr<RouteArrowsData> && routeArrowsData,
                                       ref_ptr<dp::GpuProgramManager> mng)
{
  auto & additional = m_routeAdditional[routeArrowsData->m_subrouteId];
  additional.m_arrowsData = std::move(routeArrowsData);
  BuildBuckets(additional.m_arrowsData->m_renderProperty, mng);
}

void RouteRenderer::AddPreviewRenderData(drape_ptr<CirclesPackRenderData> && renderData,
                                         ref_ptr<dp::GpuProgramManager> mng)
{
  drape_ptr<CirclesPackRenderData> data = std::move(renderData);
  ref_ptr<dp::GpuProgram> program = mng->GetProgram(gpu::CIRCLE_POINT_PROGRAM);
  program->Bind();
  data->m_bucket->GetBuffer()->Build(program);
  m_previewRenderData.push_back(std::move(data));
  m_waitForPreviewRenderData = false;

  // Save handle in the cache.
  auto & bucket = m_previewRenderData.back()->m_bucket;
  ASSERT_EQUAL(bucket->GetOverlayHandlesCount(), 1, ());
  CirclesPackHandle * handle = static_cast<CirclesPackHandle *>(bucket->GetOverlayHandle(0).get());
  handle->Clear();
  m_previewHandlesCache.push_back(std::make_pair(handle, 0));
}

void RouteRenderer::ClearRouteData()
{
  m_routeData.clear();
  m_routeAdditional.clear();
  m_hiddenSubroutes.clear();
}

void RouteRenderer::Clear()
{
  ClearRouteData();
  m_distanceFromBegin = kInvalidDistance;
}

void RouteRenderer::ClearGLDependentResources()
{
  // Here we clear only GL-dependent part of route data.
  for (auto & routeData : m_routeData)
    routeData->m_renderProperty = RouteRenderProperty();

  // All additional data (like arrows) will be regenerated, so clear them all.
  m_routeAdditional.clear();

  m_previewRenderData.clear();
  m_previewHandlesCache.clear();
  m_waitForPreviewRenderData = false;
}

void RouteRenderer::UpdateDistanceFromBegin(double distanceFromBegin)
{
  m_distanceFromBegin = distanceFromBegin;
}

void RouteRenderer::SetFollowingEnabled(bool enabled)
{
  m_followingEnabled = enabled;
}

void RouteRenderer::AddPreviewSegment(dp::DrapeID id, PreviewInfo && info)
{
  if (m_previewSegments.empty())
    m_showPreviewTimestamp = std::chrono::steady_clock::now();
  m_previewSegments.insert(std::make_pair(id, std::move(info)));
}

void RouteRenderer::RemovePreviewSegment(dp::DrapeID id)
{
  m_previewSegments.erase(id);
}

void RouteRenderer::RemoveAllPreviewSegments()
{
  m_previewSegments.clear();
}

void RouteRenderer::SetSubrouteVisibility(dp::DrapeID id, bool isVisible)
{
  if (isVisible)
    m_hiddenSubroutes.erase(id);
  else
    m_hiddenSubroutes.insert(id);
}
}  // namespace df
