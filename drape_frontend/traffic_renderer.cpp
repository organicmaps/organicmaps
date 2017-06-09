#include "drape_frontend/traffic_renderer.hpp"
#include "drape_frontend/color_constants.hpp"
#include "drape_frontend/shader_def.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/glsl_func.hpp"
#include "drape/support_manager.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "indexer/map_style_reader.hpp"
#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include <algorithm>

namespace df
{
namespace
{
df::ColorConstant const kTrafficArrowLightColor = "TrafficArrowLight";
df::ColorConstant const kTrafficArrowDarkColor = "TrafficArrowDark";
df::ColorConstant const kTrafficOutlineColor = "TrafficOutline";

int constexpr kMinVisibleArrowZoomLevel = 16;
int constexpr kRoadClass2MinVisibleArrowZoomLevel = 17;
int constexpr kOutlineMinZoomLevel = 14;

float const kTrafficArrowAspect = 128.0f / 8.0f;

std::vector<float> const kLeftWidthInPixel =
{
  // 1   2     3     4     5     6     7     8     9    10
  0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f, 0.5f,
  //11   12    13   14    15    16    17   18     19    20
  0.5f, 0.5f, 0.5f, 0.5f, 0.7f, 2.5f, 3.0f, 4.0f, 4.0f, 4.0f
};

std::vector<float> const kRightWidthInPixel =
{
  // 1   2     3     4     5     6     7     8     9    10
  2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 2.0f, 3.0f, 3.0f,
  //11  12    13    14    15    16    17    18    19     20
  3.0f, 3.0f, 4.0f, 4.0f, 3.8f, 2.5f, 3.0f, 4.0f, 4.0f, 4.0f
};

std::vector<float> const kRoadClass1WidthScalar =
{
  // 1   2     3     4     5     6     7     8     9    10
  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3,
  //11  12    13    14    15    16    17    18    19     20
  0.3, 0.3f, 0.4f, 0.5f, 0.6f, 0.6f, 1.0f, 1.0f, 1.0f, 1.0f
};

std::vector<float> const kRoadClass2WidthScalar =
{
  // 1   2     3     4     5     6     7     8     9     10
  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.3f,
  //11  12    13    14    15     16   17    18    19    20
  0.3f, 0.3f, 0.3f, 0.3f, 0.5f, 0.5f, 0.5f, 0.8f, 0.9f, 1.0f
};

std::vector<float> const kTwoWayOffsetInPixel =
{
  // 1   2     3     4     5     6     7     8     9     10
  0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
  //11  12    13    14    15     16   17    18    19    20
  0.0f, 0.5f, 0.5f, 0.5f, 1.0f, 2.5f, 2.5f, 2.5f, 2.5f, 2.5f
};

std::vector<int> const kLineDrawerRoadClass1 = {12, 13, 14};

std::vector<int> const kLineDrawerRoadClass2 = {15, 16};

float CalculateHalfWidth(ScreenBase const & screen, RoadClass const & roadClass, bool left)
{
  double zoom = 0.0;
  int index = 0;
  float lerpCoef = 0.0f;
  ExtractZoomFactors(screen, zoom, index, lerpCoef);

  std::vector<float> const * halfWidth = left ? &kLeftWidthInPixel : &kRightWidthInPixel;
  float radius = InterpolateByZoomLevels(index, lerpCoef, *halfWidth);
  if (roadClass == RoadClass::Class1)
    radius *= InterpolateByZoomLevels(index, lerpCoef, kRoadClass1WidthScalar);
  else if (roadClass == RoadClass::Class2)
    radius *= InterpolateByZoomLevels(index, lerpCoef, kRoadClass2WidthScalar);

  return radius * static_cast<float>(VisualParams::Instance().GetVisualScale());
}
}  // namespace

void TrafficRenderer::AddRenderData(ref_ptr<dp::GpuProgramManager> mng, TrafficRenderData && renderData)
{
  // Remove obsolete render data.
  TileKey const tileKey(renderData.m_tileKey);
  m_renderData.erase(remove_if(m_renderData.begin(), m_renderData.end(), [&tileKey](TrafficRenderData const & rd)
  {
    return tileKey == rd.m_tileKey && rd.m_tileKey.m_generation < tileKey.m_generation;
  }), m_renderData.end());

  // Add new render data.
  m_renderData.emplace_back(move(renderData));
  TrafficRenderData & rd = m_renderData.back();

  ref_ptr<dp::GpuProgram> program = mng->GetProgram(rd.m_state.GetProgramIndex());
  program->Bind();
  rd.m_bucket->GetBuffer()->Build(program);
}

void TrafficRenderer::OnUpdateViewport(CoverageResult const & coverage, int currentZoomLevel,
                                       buffer_vector<TileKey, 8> const & tilesToDelete)
{
  m_renderData.erase(remove_if(m_renderData.begin(), m_renderData.end(),
                               [&coverage, &currentZoomLevel, &tilesToDelete](TrafficRenderData const & rd)
  {
    return rd.m_tileKey.m_zoomLevel == currentZoomLevel &&
           (rd.m_tileKey.m_x < coverage.m_minTileX || rd.m_tileKey.m_x >= coverage.m_maxTileX ||
           rd.m_tileKey.m_y < coverage.m_minTileY || rd.m_tileKey.m_y >= coverage.m_maxTileY ||
           find(tilesToDelete.begin(), tilesToDelete.end(), rd.m_tileKey) != tilesToDelete.end());
  }), m_renderData.end());
}

void TrafficRenderer::OnGeometryReady(int currentZoomLevel)
{
  m_renderData.erase(remove_if(m_renderData.begin(), m_renderData.end(),
                               [&currentZoomLevel](TrafficRenderData const & rd)
  {
    return rd.m_tileKey.m_zoomLevel != currentZoomLevel;
  }), m_renderData.end());
}

void TrafficRenderer::RenderTraffic(ScreenBase const & screen, int zoomLevel, float opacity,
                                    ref_ptr<dp::GpuProgramManager> mng,
                                    dp::UniformValuesStorage const & commonUniforms)
{
  if (m_renderData.empty() || zoomLevel < kRoadClass0ZoomLevel)
    return;

  dp::Color const lightArrowColor = df::GetColorConstant(df::kTrafficArrowLightColor);
  dp::Color const darkArrowColor = df::GetColorConstant(df::kTrafficArrowDarkColor);
  dp::Color const outlineColor = df::GetColorConstant(df::kTrafficOutlineColor);

  for (TrafficRenderData & renderData : m_renderData)
  {
    if (renderData.m_state.GetDrawAsLine())
    {
      ref_ptr<dp::GpuProgram> program = mng->GetProgram(renderData.m_state.GetProgramIndex());
      program->Bind();
      dp::ApplyState(renderData.m_state, program);

      dp::UniformValuesStorage uniforms = commonUniforms;
      math::Matrix<float, 4, 4> const mv = renderData.m_tileKey.GetTileBasedModelView(screen);
      uniforms.SetMatrix4x4Value("modelView", mv.m_data);
      uniforms.SetFloatValue("u_opacity", opacity);
      dp::ApplyUniforms(uniforms, program);

      renderData.m_bucket->Render(true /* draw as line */);
    }
    else
    {
      // Filter by road class.
      int minVisibleArrowZoomLevel = kMinVisibleArrowZoomLevel;
      float outline = 0.0f;

      int visibleZoomLevel = kRoadClass0ZoomLevel;
      if (renderData.m_roadClass == RoadClass::Class0)
      {
        outline = (zoomLevel <= kOutlineMinZoomLevel ? 1.0f : 0.0f);
      }
      else if (renderData.m_roadClass == RoadClass::Class1)
      {
        outline = (zoomLevel <= kOutlineMinZoomLevel ? 1.0f : 0.0f);
        visibleZoomLevel = kRoadClass1ZoomLevel;
      }
      else if (renderData.m_roadClass == RoadClass::Class2)
      {
        visibleZoomLevel = kRoadClass2ZoomLevel;
        minVisibleArrowZoomLevel = kRoadClass2MinVisibleArrowZoomLevel;
      }
      if (zoomLevel < visibleZoomLevel)
        continue;

      float const leftPixelHalfWidth =
          CalculateHalfWidth(screen, renderData.m_roadClass, true /* left */);
      float const invLeftPixelLength = 1.0f / (2.0f * leftPixelHalfWidth * kTrafficArrowAspect);
      float const rightPixelHalfWidth =
          CalculateHalfWidth(screen, renderData.m_roadClass, false /* left */);
      float const kEps = 1e-5;
      if (fabs(leftPixelHalfWidth) < kEps && fabs(rightPixelHalfWidth) < kEps)
        continue;

      ref_ptr<dp::GpuProgram> program = mng->GetProgram(renderData.m_state.GetProgramIndex());
      program->Bind();
      dp::ApplyState(renderData.m_state, program);

      dp::UniformValuesStorage uniforms = commonUniforms;
      math::Matrix<float, 4, 4> const mv = renderData.m_tileKey.GetTileBasedModelView(screen);
      uniforms.SetMatrix4x4Value("modelView", mv.m_data);
      uniforms.SetFloatValue("u_opacity", opacity);
      uniforms.SetFloatValue("u_outline", outline);
      uniforms.SetFloatValue("u_lightArrowColor", lightArrowColor.GetRedF(),
                             lightArrowColor.GetGreenF(), lightArrowColor.GetBlueF());
      uniforms.SetFloatValue("u_darkArrowColor", darkArrowColor.GetRedF(),
                             darkArrowColor.GetGreenF(), darkArrowColor.GetBlueF());
      uniforms.SetFloatValue("u_outlineColor", outlineColor.GetRedF(), outlineColor.GetGreenF(),
                             outlineColor.GetBlueF());
      uniforms.SetFloatValue("u_trafficParams", leftPixelHalfWidth, rightPixelHalfWidth,
                             invLeftPixelLength,
                             zoomLevel >= minVisibleArrowZoomLevel ? 1.0f : 0.0f);
      dp::ApplyUniforms(uniforms, program);

      renderData.m_bucket->Render(false /* draw as line */);
    }
  }
}

void TrafficRenderer::ClearGLDependentResources()
{
  m_renderData.clear();
}

void TrafficRenderer::Clear(MwmSet::MwmId const & mwmId)
{
  auto removePredicate = [&mwmId](TrafficRenderData const & data) { return data.m_mwmId == mwmId; };

  m_renderData.erase(remove_if(m_renderData.begin(), m_renderData.end(), removePredicate),
                     m_renderData.end());
}

// static
float TrafficRenderer::GetTwoWayOffset(RoadClass const & roadClass, int zoomLevel)
{
  // There is no offset for class-0 roads, the offset for them is created by
  // kLeftWidthInPixel and kRightWidthInPixel.
  int const kRoadClass0MinZoomLevel = 14;
  if (roadClass == RoadClass::Class0 && zoomLevel <= kRoadClass0MinZoomLevel)
    return 0.0f;

  ASSERT_GREATER(zoomLevel, 1, ());
  ASSERT_LESS_OR_EQUAL(zoomLevel, scales::GetUpperStyleScale(), ());
  int const index = zoomLevel - 1;
  float const halfWidth = 0.5f * df::TrafficRenderer::GetPixelWidth(roadClass, zoomLevel);
  return kTwoWayOffsetInPixel[index] *
             static_cast<float>(VisualParams::Instance().GetVisualScale()) +
         halfWidth;
}

// static
float TrafficRenderer::GetPixelWidth(RoadClass const & roadClass, int zoomLevel)
{
  int width = 0;
  if (CanBeRendereredAsLine(roadClass, zoomLevel, width))
    return static_cast<float>(width);

  return GetPixelWidthInternal(roadClass, zoomLevel);
}

// static
float TrafficRenderer::GetPixelWidthInternal(RoadClass const & roadClass, int zoomLevel)
{
  ASSERT_GREATER(zoomLevel, 1, ());
  ASSERT_LESS_OR_EQUAL(zoomLevel, scales::GetUpperStyleScale(), ());
  std::vector<float> const * widthScalar = nullptr;
  if (roadClass == RoadClass::Class1)
    widthScalar = &kRoadClass1WidthScalar;
  else if (roadClass == RoadClass::Class2)
    widthScalar = &kRoadClass2WidthScalar;

  int const index = zoomLevel - 1;
  float const baseWidth = (kLeftWidthInPixel[index] + kRightWidthInPixel[index]) *
                          static_cast<float>(df::VisualParams::Instance().GetVisualScale());
  return (widthScalar != nullptr) ? (baseWidth * (*widthScalar)[index]) : baseWidth;
}

// static
bool TrafficRenderer::CanBeRendereredAsLine(RoadClass const & roadClass, int zoomLevel, int & width)
{
  if (roadClass == RoadClass::Class0)
    return false;

  std::vector<int> const * lineDrawer = nullptr;
  if (roadClass == RoadClass::Class1)
    lineDrawer = &kLineDrawerRoadClass1;
  else if (roadClass == RoadClass::Class2)
    lineDrawer = &kLineDrawerRoadClass2;

  ASSERT(lineDrawer != nullptr, ());
  auto it = find(lineDrawer->begin(), lineDrawer->end(), zoomLevel);
  if (it == lineDrawer->end())
    return false;

  width = max(1, my::rounds(TrafficRenderer::GetPixelWidthInternal(roadClass, zoomLevel)));
  return width <= dp::SupportManager::Instance().GetMaxLineWidth();
}
}  // namespace df
