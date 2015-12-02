#include "drape_frontend/gps_track_renderer.hpp"
#include "drape_frontend/visual_params.hpp"

#include "drape/glsl_func.hpp"
#include "drape/shader_def.hpp"
#include "drape/vertex_array_buffer.hpp"

#include "indexer/scales.hpp"

#include "geometry/rect_intersect.hpp"

#include "base/logging.hpp"

#include "std/algorithm.hpp"

namespace df
{

namespace
{

int const kMinVisibleZoomLevel = 15;

size_t const kAveragePointsCount = 512;

// Radius of circles depending on zoom levels.
float const kRadiusInPixel[] =
{
  // 1   2     3     4     5     6     7     8     9     10
  1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f,
  //11   12    13    14    15    16    17    18    19     20
  1.0f, 1.0f, 1.0f, 1.5f, 3.0f, 4.0f, 5.0f, 5.0f, 5.0f, 6.0f
};

double const kMinSpeed = 1.0; // meters per second
double const kAvgSpeed = 2.0; // meters per second
double const kMaxSpeed = 5.0; // meters per second
dp::Color const kMinSpeedColor = dp::Color(255, 0, 0, 255);
dp::Color const kAvgSpeedColor = dp::Color(251, 192, 45, 255);
dp::Color const kMaxSpeedColor = dp::Color(44, 120, 47, 255);
uint8_t const kMinAlpha = 50;
uint8_t const kMaxAlpha = 255;

float const kOutlineRadiusScalar = 0.3f;

double const kUnknownDistanceTime = 5 * 60; // seconds
dp::Color const kUnknownDistanceColor = dp::Color(127, 127, 127, 127);

bool GpsPointsSortPredicate(GpsTrackPoint const & pt1, GpsTrackPoint const & pt2)
{
  return pt1.m_id < pt2.m_id;
}

dp::Color InterpolateColors(dp::Color const & color1, dp::Color const & color2, double t)
{
  double const r = color1.GetRed() * (1.0 - t) + color2.GetRed() * t;
  double const g = color1.GetGreen() * (1.0 - t) + color2.GetGreen() * t;
  double const b = color1.GetBlue() * (1.0 - t) + color2.GetBlue() * t;
  double const a = color1.GetAlfa() * (1.0 - t) + color2.GetAlfa() * t;
  return dp::Color(r, g, b, a);
}

} // namespace

GpsTrackRenderer::GpsTrackRenderer(TRenderDataRequestFn const & dataRequestFn)
  : m_dataRequestFn(dataRequestFn)
  , m_needUpdate(false)
  , m_waitForRenderData(false)
  , m_startSpeed(0.0)
  , m_endSpeed(0.0)
  , m_startColor(kMaxSpeedColor)
  , m_radius(0.0f)
{
  ASSERT(m_dataRequestFn != nullptr, ());
  m_points.reserve(kAveragePointsCount);
  m_handlesCache.reserve(8);
}

float GpsTrackRenderer::CalculateRadius(ScreenBase const & screen) const
{
  double const kLog2 = log(2.0);
  double const zoomLevel = my::clamp(fabs(log(screen.GetScale()) / kLog2), 1.0, scales::UPPER_STYLE_SCALE + 1.0);
  double zoom = trunc(zoomLevel);
  int const index = zoom - 1.0;
  float const lerpCoef = zoomLevel - zoom;

  float radius = 0.0f;
  if (index < scales::UPPER_STYLE_SCALE)
    radius = kRadiusInPixel[index] + lerpCoef * (kRadiusInPixel[index + 1] - kRadiusInPixel[index]);
  else
    radius = kRadiusInPixel[scales::UPPER_STYLE_SCALE];

  return radius * VisualParams::Instance().GetVisualScale();
}

void GpsTrackRenderer::AddRenderData(ref_ptr<dp::GpuProgramManager> mng,
                                     drape_ptr<GpsTrackRenderData> && renderData)
{
  drape_ptr<GpsTrackRenderData> data = move(renderData);
  ref_ptr<dp::GpuProgram> program = mng->GetProgram(gpu::TRACK_POINT_PROGRAM);
  program->Bind();
  data->m_bucket->GetBuffer()->Build(program);
  m_renderData.push_back(move(data));
  m_waitForRenderData = false;
}

void GpsTrackRenderer::UpdatePoints(vector<GpsTrackPoint> const & toAdd, vector<uint32_t> const & toRemove)
{
  if (!toRemove.empty())
  {
    auto removePredicate = [&toRemove](GpsTrackPoint const & pt)
    {
      return find(toRemove.begin(), toRemove.end(), pt.m_id) != toRemove.end();
    };
    m_points.erase(remove_if(m_points.begin(), m_points.end(), removePredicate), m_points.end());
  }

  if (!toAdd.empty())
  {
    ASSERT(is_sorted(toAdd.begin(), toAdd.end(), GpsPointsSortPredicate), ());
    if (!m_points.empty())
      ASSERT(GpsPointsSortPredicate(m_points.back(), toAdd.front()), ());
    m_points.insert(m_points.end(), toAdd.begin(), toAdd.end());
  }

  m_needUpdate = true;
}

double GpsTrackRenderer::CalculateTrackLength() const
{
  double len = 0.0;
  for (size_t i = 0; i + 1 < m_points.size(); i++)
    len += (m_points[i + 1].m_point - m_points[i].m_point).Length();

  return len;
}

void GpsTrackRenderer::UpdateSpeedsAndColors()
{
  m_startSpeed = 0.0;
  m_endSpeed = 0.0;
  for (size_t i = 0; i < m_points.size(); i++)
  {
    if (m_points[i].m_speedMPS < m_startSpeed)
      m_startSpeed = m_points[i].m_speedMPS;

    if (m_points[i].m_speedMPS > m_endSpeed)
      m_endSpeed = m_points[i].m_speedMPS;
  }

  m_startSpeed = max(m_startSpeed, 0.0);
  m_endSpeed = max(m_endSpeed, 0.0);

  double const delta = m_endSpeed - m_startSpeed;
  if (delta <= kMinSpeed)
    m_startColor = kMaxSpeedColor;
  else if (delta <= kAvgSpeed)
    m_startColor = kAvgSpeedColor;
  else
    m_startColor = kMinSpeedColor;

  m_startSpeed = max(m_startSpeed, kMinSpeed);
  m_endSpeed = min(m_endSpeed, kMaxSpeed);

  double const kBias = 0.01;
  if (fabs(m_endSpeed - m_startSpeed) < 1e-5)
    m_endSpeed += kBias;
}

size_t GpsTrackRenderer::GetAvailablePointsCount() const
{
  size_t pointsCount = 0;
  for (size_t i = 0; i < m_renderData.size(); i++)
    pointsCount += m_renderData[i]->m_pointsCount;

  return pointsCount;
}

double GpsTrackRenderer::PlacePoints(size_t & cacheIndex, bool unknownDistance,
                                     m2::PointD const & start, m2::PointD const & end,
                                     double startSpeed, double endSpeed,
                                     float radius, double diameterMercator,
                                     double offset, double trackLengthMercator,
                                     double lengthFromStart, bool & gap)
{
  double const kDistanceScalar = 0.65;

  m2::PointD const delta = end - start;
  double const length = delta.Length();
  m2::PointD const dir = delta.Normalize();
  double pos = offset;
  while (pos < length)
  {
    if (gap)
    {
      double const dist = pos + diameterMercator * 0.5;
      dp::Color color = kUnknownDistanceColor;
      if (!unknownDistance)
      {
        double const td = my::clamp(dist / length, 0.0, 1.0);
        double const speed = max(startSpeed * (1.0 - td) + endSpeed * td, 0.0);
        double const ts = my::clamp((speed - m_startSpeed) / (m_endSpeed - m_startSpeed), 0.0, 1.0);
        color = InterpolateColors(m_startColor, kMaxSpeedColor, ts);
        double const ta = my::clamp((lengthFromStart + dist) / trackLengthMercator, 0.0, 1.0);
        double const alpha = kMinAlpha * (1.0 - ta) + kMaxAlpha * ta;
        color = dp::Color(color.GetRed(), color.GetGreen(), color.GetBlue(), alpha);
      }

      m2::PointD const p = start + dir * dist;
      m_handlesCache[cacheIndex].first->SetPoint(m_handlesCache[cacheIndex].second, p, radius, color);
      m_handlesCache[cacheIndex].second++;
      if (m_handlesCache[cacheIndex].second >= m_handlesCache[cacheIndex].first->GetPointsCount())
        cacheIndex++;

      if (cacheIndex >= m_handlesCache.size())
      {
        m_dataRequestFn(kAveragePointsCount);
        m_waitForRenderData = true;
        return -1.0;
      }
    }
    gap = !gap;
    pos += (diameterMercator * kDistanceScalar);
  }

  return pos - length;
}

void GpsTrackRenderer::RenderTrack(ScreenBase const & screen, int zoomLevel,
                                   ref_ptr<dp::GpuProgramManager> mng,
                                   dp::UniformValuesStorage const & commonUniforms)
{
  if (zoomLevel < kMinVisibleZoomLevel)
    return;

  if (m_needUpdate)
  {
    // Skip rendering if there is no any point.
    if (m_points.empty())
    {
      m_needUpdate = false;
      return;
    }

    // Check if there are render data.
    if (m_renderData.empty() && !m_waitForRenderData)
    {
      m_dataRequestFn(kAveragePointsCount);
      m_waitForRenderData = true;
    }

    if (m_waitForRenderData)
      return;

    m_radius = CalculateRadius(screen);
    float const diameter = 2.0f * m_radius;
    float const currentScaleGtoP = 1.0f / screen.GetScale();
    double const trackLengthMercator = CalculateTrackLength();

    // Update points' positions and colors.
    ASSERT(!m_renderData.empty(), ());
    m_handlesCache.clear();
    for (size_t i = 0; i < m_renderData.size(); i++)
    {
      ASSERT_EQUAL(m_renderData[i]->m_bucket->GetOverlayHandlesCount(), 1, ());
      GpsTrackHandle * handle = static_cast<GpsTrackHandle*>(m_renderData[i]->m_bucket->GetOverlayHandle(0).get());
      handle->Clear();
      m_handlesCache.push_back(make_pair(handle, 0));
    }
    UpdateSpeedsAndColors();

    size_t cacheIndex = 0;
    double lengthFromStart = 0.0;
    if (m_points.size() == 1)
    {
      m_handlesCache[cacheIndex].first->SetPoint(0, m_points.front().m_point, m_radius, kMaxSpeedColor);
      m_handlesCache[cacheIndex].second++;
    }
    else
    {
      bool gap = true;
      double const diameterMercator = diameter / currentScaleGtoP;
      double offset = 0.0;
      for (size_t i = 0; i + 1 < m_points.size(); i++)
      {
        // Clip points outside visible rect.
        m2::PointD p1 = m_points[i].m_point;
        m2::PointD p2 = m_points[i + 1].m_point;
        double const dist = (m_points[i + 1].m_point - m_points[i].m_point).Length();
        if (!m2::Intersect(screen.ClipRect(), p1, p2))
        {
          lengthFromStart += dist;
          offset = 0.0;
          continue;
        }

        // Check points equality.
        if (p1.EqualDxDy(p2, 1e-5))
        {
          lengthFromStart += dist;
          continue;
        }

        // Interpolate speeds.
        double const startOffset = (p1 - m_points[i].m_point).Length();
        double const t1 = startOffset / dist;
        double const t2 = 1.0 - (m_points[i + 1].m_point - p2).Length() / dist;
        double startSpeed = m_points[i].m_speedMPS * (1.0 - t1) + m_points[i + 1].m_speedMPS * t1;
        double endSpeed = m_points[i].m_speedMPS * (1.0 - t2) + m_points[i + 1].m_speedMPS * t2;

        bool const unknownDist = (m_points[i + 1].m_timestamp - m_points[i].m_timestamp) > kUnknownDistanceTime;

        // Place points.
        offset = PlacePoints(cacheIndex, unknownDist, p1, p2, startSpeed, endSpeed, m_radius, diameterMercator,
                             offset, trackLengthMercator, lengthFromStart + startOffset, gap);
        if (offset < 0.0)
          return;

        lengthFromStart += dist;
      }
    }
    m_needUpdate = false;
  }

  if (m_handlesCache.empty() || m_handlesCache.front().second == 0)
    return;

  GLFunctions::glClearDepth();

  ASSERT_EQUAL(m_renderData.size(), m_handlesCache.size(), ());

  // Render points.
  dp::UniformValuesStorage uniforms = commonUniforms;
  uniforms.SetFloatValue("u_opacity", 1.0f);
  uniforms.SetFloatValue("u_radiusShift", m_radius * kOutlineRadiusScalar);
  ref_ptr<dp::GpuProgram> program = mng->GetProgram(gpu::TRACK_POINT_PROGRAM);
  program->Bind();

  ASSERT_GREATER(m_renderData.size(), 0, ());
  dp::ApplyState(m_renderData.front()->m_state, program);
  dp::ApplyUniforms(uniforms, program);

  for (size_t i = 0; i < m_renderData.size(); i++)
    if (m_handlesCache[i].second != 0)
      m_renderData[i]->m_bucket->Render(screen);
}

void GpsTrackRenderer::Update()
{
  m_needUpdate = true;
}

void GpsTrackRenderer::Clear()
{
  m_points.clear();
  m_needUpdate = true;
}

} // namespace df

