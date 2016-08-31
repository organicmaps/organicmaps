#include "map/generate_chart.hpp"

#include "base/assert.hpp"

#include "std/algorithm.hpp"
#include "std/fstream.hpp"

#include "3party/agg/agg_conv_curve.h"
#include "3party/agg/agg_conv_stroke.h"
#include "3party/agg/agg_path_storage.h"
#include "3party/agg/agg_pixfmt_rgba.h"
#include "3party/agg/agg_rasterizer_scanline_aa.h"
#include "3party/agg/agg_renderer_primitives.h"
#include "3party/agg/agg_renderer_scanline.h"
#include "3party/agg/agg_scanline_p.h"

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

  static AGG_INLINE void blend_pix(unsigned op, TValueType * p, unsigned cr, unsigned cg,
                                   unsigned cb, unsigned ca, unsigned cover)
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
}  // namespace

void NormalizeChartData(deque<double> const & distanceDataM, feature::TAltitudes const & altitudeDataM,
                        size_t resultPointCount, vector<double> & uniformAltitudeDataM)
{
  uniformAltitudeDataM.clear();
  if (distanceDataM.empty() || resultPointCount == 0 || distanceDataM.size() != altitudeDataM.size())
    return;

  if (!is_sorted(distanceDataM.cbegin(), distanceDataM.cend()))
  {
    LOG(LERROR, ("Route segment distances are not sorted."));
    return;
  }

  uniformAltitudeDataM.resize(resultPointCount);
  double const routeLenM = distanceDataM.back();
  double const stepLenM = routeLenM / static_cast<double> (resultPointCount - 1);

  auto const calculateAltitude = [&](double distFormStartM)
  {
    if (distFormStartM <= distanceDataM.front() )
      return static_cast<double>(altitudeDataM.front());
    if (distFormStartM >= distanceDataM.back())
      return static_cast<double>(altitudeDataM.back());

    auto const lowerIt = lower_bound(distanceDataM.cbegin(), distanceDataM.cend(), distFormStartM);
    if (lowerIt == distanceDataM.cbegin())
      return static_cast<double>(altitudeDataM.front());
    if (lowerIt == distanceDataM.cend())
      return static_cast<double>(altitudeDataM.back());

    size_t const nextPointIdx = distance(distanceDataM.cbegin(), lowerIt);
    CHECK_LESS(0, nextPointIdx, ());
    size_t const prevPointIdx = nextPointIdx - 1;

    if (distanceDataM[prevPointIdx] == distanceDataM[nextPointIdx])
      return static_cast<double>(altitudeDataM[prevPointIdx]);

    double const k = (altitudeDataM[nextPointIdx] - altitudeDataM[prevPointIdx]) /
        (distanceDataM[nextPointIdx] - distanceDataM[prevPointIdx]);
    return static_cast<double>(altitudeDataM[prevPointIdx]) + k * (distFormStartM - distanceDataM[prevPointIdx]);
  };

  uniformAltitudeDataM.resize(resultPointCount);
  for (size_t i = 0; i < resultPointCount; ++i)
    uniformAltitudeDataM[i] = calculateAltitude(static_cast<double>(i) * stepLenM);
}

void GenerateYAxisChartData(size_t height, double minMetersPerPxl,
                            vector<double> const & altitudeDataM, vector<double> & yAxisDataPxl)
{
  uint32_t constexpr kHeightIndentPxl = 2.0;
  yAxisDataPxl.clear();
  uint32_t heightIndent = kHeightIndentPxl;
  if (height <= 2 * kHeightIndentPxl)
  {
    LOG(LERROR, ("Chart height is less then 2 * kHeightIndentPxl (", 2 * kHeightIndentPxl, ")"));
    heightIndent = 0;
  }

  auto const minMaxAltitudeIt = minmax_element(altitudeDataM.begin(), altitudeDataM.end());
  double const minAltM = *minMaxAltitudeIt.first;
  double const maxAltM = *minMaxAltitudeIt.second;
  double const deltaAltM = maxAltM - minAltM;
  uint32_t const drawHeightPxl = height - 2 * heightIndent;
  double const meterPerPxl = max(minMetersPerPxl, deltaAltM / static_cast<double>(drawHeightPxl));
  if (meterPerPxl == 0.0)
  {
    LOG(LERROR, ("meterPerPxl == 0.0"));
    return;
  }

  size_t const altitudeDataSz = altitudeDataM.size();
  yAxisDataPxl.resize(altitudeDataSz);
  for (size_t i = 0; i < altitudeDataSz; ++i)
    yAxisDataPxl[i] = height - heightIndent -  (altitudeDataM[i] - minAltM) / meterPerPxl;
}

void GenerateChartByPoints(size_t width, size_t height, vector<m2::PointD> const & geometry,
                          bool day, vector<uint8_t> & frameBuffer)
{
  frameBuffer.clear();
  if (width == 0 || height == 0 || geometry.empty())
    return;

  agg::rgba8 const kBackgroundColor = agg::rgba8(255, 255, 255, 0);
  agg::rgba8 const kLineColor = day ? agg::rgba8(30, 150, 240, 255) : agg::rgba8(255, 230, 140, 255);
  agg::rgba8 const kCurveColor = day ? agg::rgba8(30, 150, 240, 20) : agg::rgba8(255, 230, 140, 20);
  double constexpr kLineWidthPxl = 2.0;
  uint32_t constexpr kBPP = 4;

  using TBlender = BlendAdaptor<agg::rgba8, agg::order_rgba>;
  using TPixelFormat = agg::pixfmt_custom_blend_rgba<TBlender, agg::rendering_buffer>;
  using TBaseRenderer = agg::renderer_base<TPixelFormat>;
  using TPrimitivesRenderer = agg::renderer_primitives<TBaseRenderer>;
  using TSolidRenderer = agg::renderer_scanline_aa_solid<TBaseRenderer>;
  using TPath = agg::poly_container_adaptor<vector<m2::PointD>>;
  using TStroke = agg::conv_stroke<TPath>;

  agg::rendering_buffer renderBuffer;
  TPixelFormat pixelFormat(renderBuffer, agg::comp_op_src_over);
  TBaseRenderer m_baseRenderer(pixelFormat);

  frameBuffer.assign(width * kBPP * height, 0);
  renderBuffer.attach(&frameBuffer[0], static_cast<unsigned int>(width),
                      static_cast<unsigned int>(height),
                      static_cast<int>(width * kBPP));
  m_baseRenderer.reset_clipping(true);
  unsigned op = pixelFormat.comp_op();
  pixelFormat.comp_op(agg::comp_op_src);
  m_baseRenderer.clear(kBackgroundColor);
  pixelFormat.comp_op(op);

  agg::rasterizer_scanline_aa<> rasterizer;
  rasterizer.clip_box(0, 0, width, height);

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
  agg::render_scanlines_aa_solid(rasterizer, scanline, m_baseRenderer, kCurveColor);

  // Chart line.
  TPath path_adaptor(geometry, false);
  TStroke stroke(path_adaptor);
  stroke.width(kLineWidthPxl);
  stroke.line_cap(agg::round_cap);
  stroke.line_join(agg::round_join);

  rasterizer.add_path(stroke);
  agg::render_scanlines_aa_solid(rasterizer, scanline, m_baseRenderer, kLineColor);
}

void GenerateChart(size_t width, size_t height,
                   deque<double> const & distanceDataM, feature::TAltitudes const & altitudeDataM,
                   bool day, vector<uint8_t> & frameBuffer)
{
  frameBuffer.clear();
  if (altitudeDataM.empty() || distanceDataM.size() != altitudeDataM.size())
    return;

  vector<double> uniformAltitudeDataM;
  NormalizeChartData(distanceDataM, altitudeDataM, width, uniformAltitudeDataM);
  if (uniformAltitudeDataM.empty())
    return;

  vector<double> yAxisDataPxl;
  GenerateYAxisChartData(height, 1.0 /* minMetersPerPxl */, uniformAltitudeDataM, yAxisDataPxl);

  size_t const uniformAltitudeDataSz = yAxisDataPxl.size();
  double const oneSegLenPix = static_cast<double>(width) / (uniformAltitudeDataSz - 1);
  vector<m2::PointD> geometry(uniformAltitudeDataSz);
  for (size_t i = 0; i < uniformAltitudeDataSz; ++i)
    geometry[i] = m2::PointD(i * oneSegLenPix, yAxisDataPxl[i]);

  GenerateChartByPoints(width, height, geometry, day, frameBuffer);
}
