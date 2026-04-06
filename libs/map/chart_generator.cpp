#include "map/chart_generator.hpp"

#include "map/elevation_info.hpp"

#include "indexer/map_style_reader.hpp"

#include "base/assert.hpp"
#include "base/logging.hpp"

#include "3party/agg/agg_conv_curve.h"
#include "3party/agg/agg_conv_stroke.h"
#include "3party/agg/agg_path_storage.h"
#include "3party/agg/agg_pixfmt_rgba.h"
#include "3party/agg/agg_rasterizer_scanline_aa.h"
#include "3party/agg/agg_renderer_scanline.h"
#include "3party/agg/agg_scanline_p.h"

#include <algorithm>

namespace
{
template <class Color, class Order>
struct BlendAdaptor
{
  using order_type = Order;
  using color_type = Color;
  using TValueType = typename color_type::value_type;
  using TCalcType = typename color_type::calc_type;

  enum
  {
    ScaleShift = color_type::base_shift,
    ScaleMask = color_type::base_mask,
  };

  static AGG_INLINE void blend_pix(unsigned op, TValueType * p, unsigned cr, unsigned cg, unsigned cb, unsigned ca,
                                   unsigned cover)
  {
    using TBlendTable = agg::comp_op_table_rgba<Color, Order>;
    if (p[Order::A])
    {
      TBlendTable::g_comp_op_func[op](p, (cr * ca + ScaleMask) >> ScaleShift, (cg * ca + ScaleMask) >> ScaleShift,
                                      (cb * ca + ScaleMask) >> ScaleShift, ca, cover);
    }
    else
    {
      TBlendTable::g_comp_op_func[op](p, cr, cg, cb, ca, cover);
    }
  }
};

agg::rgba8 GetLineColor(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleCount: LOG(LERROR, ("Wrong map style param."));  // fallthrough
  case MapStyleDefaultDark:
  case MapStyleVehicleDark:
  case MapStyleOutdoorsDark: return agg::rgba8(255, 230, 140, 255);
  case MapStyleDefaultLight:
  case MapStyleVehicleLight:
  case MapStyleOutdoorsLight:
  case MapStyleMerged: return agg::rgba8(30, 150, 240, 255);
  }
  UNREACHABLE();
}

agg::rgba8 GetCurveColor(MapStyle mapStyle)
{
  switch (mapStyle)
  {
  case MapStyleCount:
    LOG(LERROR, ("Wrong map style param."));
    [[fallthrough]];
    // No need break or return here.
  case MapStyleDefaultDark:
  case MapStyleVehicleDark:
  case MapStyleOutdoorsDark: return agg::rgba8(255, 230, 140, 20);
  case MapStyleDefaultLight:
  case MapStyleVehicleLight:
  case MapStyleOutdoorsLight:
  case MapStyleMerged: return agg::rgba8(30, 150, 240, 20);
  }
  UNREACHABLE();
}
}  // namespace

void ChartGenerator::ScaleChartData(std::vector<double> & chartData, double scale)
{
  for (size_t i = 0; i < chartData.size(); ++i)
    chartData[i] *= scale;
}

void ChartGenerator::ShiftChartData(std::vector<double> & chartData, double shift)
{
  for (size_t i = 0; i < chartData.size(); ++i)
    chartData[i] += shift;
}

void ChartGenerator::ReflectChartData(std::vector<double> & chartData)
{
  for (size_t i = 0; i < chartData.size(); ++i)
    chartData[i] = -chartData[i];
}

void ChartGenerator::NormalizeAltitudes(size_t resultPointCount, std::vector<double> & uniformAltitudeDataM) const
{
  ASSERT(resultPointCount > 0, ());

  double const totalDistance = m_info.GetLength();
  double const stepLen = resultPointCount <= 1 ? 0.0 : totalDistance / (resultPointCount - 1);
  uniformAltitudeDataM.resize(resultPointCount);

  // Sequential scan: iterate through ElevationInfo points while generating uniform samples.
  // Both sample distances and point distances are monotonically increasing.
  size_t sampleIdx = 0;
  double prevDist = 0;
  double prevAlt = 0;
  bool hasPrev = false;

  m_info.ForEachPoint([&](double dist, geometry::Altitude alt)
  {
    if (!hasPrev)
    {
      prevDist = dist;
      prevAlt = alt;
      hasPrev = true;
    }

    while (sampleIdx < resultPointCount)
    {
      double const sampleDist = sampleIdx * stepLen;
      if (sampleDist > dist + kDistanceEps)
        break;

      double const segLen = dist - prevDist;
      if (segLen < kDistanceEps)
        uniformAltitudeDataM[sampleIdx] = prevAlt;
      else
      {
        double const k = (static_cast<double>(alt) - prevAlt) / segLen;
        uniformAltitudeDataM[sampleIdx] = prevAlt + k * (sampleDist - prevDist);
      }
      ++sampleIdx;
    }

    prevDist = dist;
    prevAlt = alt;
  });

  // Fill remaining samples with last altitude (floating point edge case).
  while (sampleIdx < resultPointCount)
    uniformAltitudeDataM[sampleIdx++] = prevAlt;

  // Ensure the endpoint matches the final point's altitude
  // (handles duplicate terminal distances from zero-length segments).
  if (hasPrev)
  {
    ASSERT(!uniformAltitudeDataM.empty(), ());
    uniformAltitudeDataM.back() = prevAlt;
  }
}

bool ChartGenerator::GenerateYAxisChartData(uint32_t height, double minMetersPerPxl,
                                            std::vector<double> const & altitudeDataM,
                                            std::vector<double> & yAxisDataPxl)
{
  ASSERT(!altitudeDataM.empty(), ());

  uint32_t constexpr kHeightIndentPxl = 2;
  uint32_t heightIndentPxl = kHeightIndentPxl;
  if (height <= 2 * kHeightIndentPxl)
  {
    LOG(LERROR, ("Chart height is less or equal than 2 * kHeightIndentPxl (", 2 * kHeightIndentPxl, ")"));
    heightIndentPxl = 0;
  }

  auto const minMaxAltitudeIt = std::minmax_element(altitudeDataM.begin(), altitudeDataM.end());
  double const minAltM = *minMaxAltitudeIt.first;
  double const maxAltM = *minMaxAltitudeIt.second;
  double const deltaAltM = maxAltM - minAltM;
  uint32_t const drawHeightPxl = height - 2 * heightIndentPxl;
  double const metersPerPxl = std::max(minMetersPerPxl, deltaAltM / static_cast<double>(drawHeightPxl));
  if (metersPerPxl == 0.0)
  {
    LOG(LERROR, ("metersPerPxl == 0.0"));
    return false;
  }
  // int avoids double errors which make freeHeightSpacePxl slightly less than zero.
  int const deltaAltPxl = deltaAltM / metersPerPxl;
  double const freeHeightSpacePxl = drawHeightPxl - deltaAltPxl;
  if (freeHeightSpacePxl < 0 || freeHeightSpacePxl > drawHeightPxl)
  {
    LOG(LERROR, ("Number of pixels free of chart points (", freeHeightSpacePxl,
                 ") is below zero or greater than the number of pixels for the chart (", drawHeightPxl, ")."));
    return false;
  }

  double const maxAltPxl = maxAltM / metersPerPxl;
  yAxisDataPxl.assign(altitudeDataM.cbegin(), altitudeDataM.cend());
  ScaleChartData(yAxisDataPxl, 1.0 / metersPerPxl);
  ReflectChartData(yAxisDataPxl);
  ShiftChartData(yAxisDataPxl, maxAltPxl + heightIndentPxl + freeHeightSpacePxl / 2.0);

  return true;
}

void ChartGenerator::GenerateByPoints(uint32_t width, uint32_t height, std::vector<m2::PointD> const & geometry,
                                      MapStyle mapStyle, std::vector<uint8_t> & frameBuffer)
{
  ASSERT(width > 0 && height > 0, ());
  frameBuffer.clear();

  agg::rgba8 const kBackgroundColor = agg::rgba8(255, 255, 255, 0);
  agg::rgba8 const kLineColor = GetLineColor(mapStyle);
  agg::rgba8 const kCurveColor = GetCurveColor(mapStyle);
  double constexpr kLineWidthPxl = 2.0;

  using TBlender = BlendAdaptor<agg::rgba8, agg::order_rgba>;
  using TPixelFormat = agg::pixfmt_custom_blend_rgba<TBlender, agg::rendering_buffer>;
  using TBaseRenderer = agg::renderer_base<TPixelFormat>;
  using TPath = agg::poly_container_adaptor<std::vector<m2::PointD>>;
  using TStroke = agg::conv_stroke<TPath>;

  agg::rendering_buffer renderBuffer;
  TPixelFormat pixelFormat(renderBuffer, agg::comp_op_src_over);
  TBaseRenderer baseRenderer(pixelFormat);

  frameBuffer.assign(width * kBPP * height, 0);
  renderBuffer.attach(&frameBuffer[0], static_cast<unsigned>(width), static_cast<unsigned>(height),
                      static_cast<int>(width * kBPP));

  // Background.
  baseRenderer.reset_clipping(true);
  unsigned const op = pixelFormat.comp_op();
  pixelFormat.comp_op(agg::comp_op_src);
  baseRenderer.clear(kBackgroundColor);
  pixelFormat.comp_op(op);

  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.clip_box(0, 0, width, height);

  // No chart line to draw.
  if (geometry.empty())
    return;

  // Polygon under chart line.
  agg::path_storage underChartGeometryPath;
  underChartGeometryPath.move_to(geometry.front().x, static_cast<double>(height));
  for (auto const & p : geometry)
    underChartGeometryPath.line_to(p.x, p.y);
  underChartGeometryPath.line_to(geometry.back().x, static_cast<double>(height));
  underChartGeometryPath.close_polygon();

  agg::conv_curve<agg::path_storage> curve(underChartGeometryPath);
  rasterizer.add_path(curve);
  agg::scanline32_p8 scanline;
  agg::render_scanlines_aa_solid(rasterizer, scanline, baseRenderer, kCurveColor);

  // Chart line.
  TPath path_adaptor(geometry, false);
  TStroke stroke(path_adaptor);
  stroke.width(kLineWidthPxl);
  stroke.line_cap(agg::round_cap);
  stroke.line_join(agg::round_join);

  rasterizer.add_path(stroke);
  agg::render_scanlines_aa_solid(rasterizer, scanline, baseRenderer, kLineColor);
}

bool ChartGenerator::Generate(uint32_t width, uint32_t height, MapStyle mapStyle,
                              std::vector<uint8_t> & frameBuffer) const
{
  if (width == 0 || height == 0 || m_info.IsEmpty())
    return false;

  std::vector<double> uniformAltitudeDataM;
  NormalizeAltitudes(width, uniformAltitudeDataM);

  std::vector<double> yAxisDataPxl;
  if (!GenerateYAxisChartData(height, 1.0 /* minMetersPerPxl */, uniformAltitudeDataM, yAxisDataPxl))
    return false;

  size_t const sz = yAxisDataPxl.size();
  std::vector<m2::PointD> geometry(sz);
  if (sz != 0)
  {
    double const oneSegLenPix = static_cast<double>(width) / (sz == 1 ? 1 : (sz - 1));
    for (size_t i = 0; i < sz; ++i)
      geometry[i] = m2::PointD(i * oneSegLenPix, yAxisDataPxl[i]);
  }

  GenerateByPoints(width, height, geometry, mapStyle, frameBuffer);
  return true;
}

bool ChartGenerator::Generate(uint32_t width, uint32_t height, std::vector<uint8_t> & frameBuffer) const
{
  return Generate(width, height, GetStyleReader().GetCurrentStyle(), frameBuffer);
}
