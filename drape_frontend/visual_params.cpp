#include "drape_frontend/visual_params.hpp"

#include "base/macros.hpp"
#include "base/math.hpp"
#include "base/assert.hpp"

#include "geometry/mercator.hpp"

#include "indexer/scales.hpp"

#include "std/algorithm.hpp"
#include "std/limits.hpp"
#include "std/utility.hpp"

namespace df
{

namespace
{

static VisualParams g_VizParams;

typedef pair<string, double> visual_scale_t;

} // namespace

#ifdef DEBUG
static bool g_isInited = false;
#define RISE_INITED g_isInited = true
#define ASSERT_INITED ASSERT(g_isInited, ())
#else
#define RISE_INITED
#define ASSERT_INITED
#endif

double const VisualParams::kMdpiScale = 1.0;
double const VisualParams::kHdpiScale = 1.5;
double const VisualParams::kXhdpiScale = 2.0;
double const VisualParams::k6plusScale = 2.4;
double const VisualParams::kXxhdpiScale = 3.0;

void VisualParams::Init(double vs, uint32_t tileSize)
{
  g_VizParams.m_tileSize = tileSize;
  g_VizParams.m_visualScale = vs;

  // Here we set up glyphs rendering parameters separately for high-res and low-res screens.
  if (vs <= 1.0)
    g_VizParams.m_glyphVisualParams = { 0.48f, 0.08f, 0.2f, 0.01f, 0.49f, 0.04f };
  else
    g_VizParams.m_glyphVisualParams = { 0.5f, 0.06f, 0.2f, 0.01f, 0.49f, 0.04f };

  RISE_INITED;
}

uint32_t VisualParams::GetGlyphSdfScale() const
{
  return (m_visualScale <= 1.0) ? 3 : 4;
}

uint32_t VisualParams::GetGlyphBaseSize() const
{
  return 22;
}

double VisualParams::GetFontScale() const
{
  return m_fontScale;
}

void VisualParams::SetFontScale(double fontScale)
{
  m_fontScale = fontScale;
}

VisualParams & VisualParams::Instance()
{
  ASSERT_INITED;
  return g_VizParams;
}

string const & VisualParams::GetResourcePostfix(double visualScale)
{
  static visual_scale_t postfixes[] =
  {
    make_pair("mdpi", kMdpiScale),
    make_pair("hdpi", kHdpiScale),
    make_pair("xhdpi", kXhdpiScale),
    make_pair("xxhdpi", kXxhdpiScale),
    make_pair("6plus", k6plusScale),
  };

  // Looking for the nearest available scale.
  int postfixIndex = -1;
  double minValue = numeric_limits<double>::max();
  for (int i = 0; i < static_cast<int>(ARRAY_SIZE(postfixes)); i++)
  {
    double val = fabs(postfixes[i].second - visualScale);
    if (val < minValue)
    {
      minValue = val;
      postfixIndex = i;
    }
  }

  ASSERT_GREATER_OR_EQUAL(postfixIndex, 0, ());
  return postfixes[postfixIndex].first;
}

string const & VisualParams::GetResourcePostfix() const
{
  return VisualParams::GetResourcePostfix(m_visualScale);
}

double VisualParams::GetVisualScale() const
{
  return m_visualScale;
}

uint32_t VisualParams::GetTileSize() const
{
  return m_tileSize;
}

uint32_t VisualParams::GetTouchRectRadius() const
{
  float const kRadiusInPixels = 20.0f;
  return kRadiusInPixels * GetVisualScale();
}

double VisualParams::GetDragThreshold() const
{
  double const kDragThresholdInPixels = 10.0;
  return kDragThresholdInPixels * GetVisualScale();
}

double VisualParams::GetScaleThreshold() const
{
  double const kScaleThresholdInPixels = 2.0;
  return kScaleThresholdInPixels * GetVisualScale();
}

VisualParams::GlyphVisualParams const & VisualParams::GetGlyphVisualParams() const
{
  return m_glyphVisualParams;
}

VisualParams::VisualParams()
  : m_tileSize(0)
  , m_visualScale(0.0)
  , m_fontScale(1.0)
{
}

m2::RectD const & GetWorldRect()
{
  static m2::RectD const worldRect = MercatorBounds::FullRect();
  return worldRect;
}

int GetTileScaleBase(ScreenBase const & s, uint32_t tileSize)
{
  ScreenBase tmpS = s;
  tmpS.Rotate(-tmpS.GetAngle());

  // slightly smaller than original to produce "antialiasing" effect using bilinear filtration.
  int const halfSize = static_cast<int>(tileSize / 1.05 / 2.0);

  m2::RectD glbRect;
  m2::PointD const pxCenter = tmpS.PixelRect().Center();
  tmpS.PtoG(m2::RectD(pxCenter - m2::PointD(halfSize, halfSize),
                      pxCenter + m2::PointD(halfSize, halfSize)),
            glbRect);

  return GetTileScaleBase(glbRect);
}

int GetTileScaleBase(ScreenBase const & s)
{
  return GetTileScaleBase(s, VisualParams::Instance().GetTileSize());
}

int GetTileScaleBase(m2::RectD const & r)
{
  double const sz = max(r.SizeX(), r.SizeY());
  return max(1, my::rounds(log((MercatorBounds::maxX - MercatorBounds::minX) / sz) / log(2.0)));
}

int GetTileScaleIncrement(uint32_t tileSize, double visualScale)
{
  return log(tileSize / 256.0 / visualScale) / log(2.0);
}

int GetTileScaleIncrement()
{
  VisualParams const & p = VisualParams::Instance();
  return GetTileScaleIncrement(p.GetTileSize(), p.GetVisualScale());
}

m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center, uint32_t tileSize, double visualScale)
{
  // +1 - we will calculate half length for each side
  double const factor = 1 << (max(1, drawScale - GetTileScaleIncrement(tileSize, visualScale)) + 1);

  double const len = (MercatorBounds::maxX - MercatorBounds::minX) / factor;

  return m2::RectD(MercatorBounds::ClampX(center.x - len),
                   MercatorBounds::ClampY(center.y - len),
                   MercatorBounds::ClampX(center.x + len),
                   MercatorBounds::ClampY(center.y + len));
}

m2::RectD GetRectForDrawScale(int drawScale, m2::PointD const & center)
{
  VisualParams const & p = VisualParams::Instance();
  return GetRectForDrawScale(drawScale, center, p.GetTileSize(), p.GetVisualScale());
}

m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center, uint32_t tileSize, double visualScale)
{
  return GetRectForDrawScale(my::rounds(drawScale), center, tileSize, visualScale);
}

m2::RectD GetRectForDrawScale(double drawScale, m2::PointD const & center)
{
  return GetRectForDrawScale(my::rounds(drawScale), center);
}

uint32_t CalculateTileSize(uint32_t screenWidth, uint32_t screenHeight)
{
  uint32_t const maxSz = max(screenWidth, screenHeight);

  // we're calculating the tileSize based on (maxSz > 1024 ? rounded : ceiled)
  // to the nearest power of two value of the maxSz

  int const ceiledSz = 1 << static_cast<int>(ceil(log(double(maxSz + 1)) / log(2.0)));
  int res = 0;

  if (maxSz < 1024)
  {
    res = ceiledSz;
  }
  else
  {
    int const flooredSz = ceiledSz / 2;
    // rounding to the nearest power of two.
    if (ceiledSz - maxSz < maxSz - flooredSz)
      res = ceiledSz;
    else
      res = flooredSz;
  }

#ifndef OMIM_OS_DESKTOP
  return static_cast<uint32_t>(my::clamp(res / 2, 256, 1024));
#else
  return static_cast<uint32_t>(my::clamp(res / 2, 512, 1024));
#endif
}

int GetDrawTileScale(int baseScale, uint32_t tileSize, double visualScale)
{
  return max(1, baseScale + GetTileScaleIncrement(tileSize, visualScale));
}

int GetDrawTileScale(ScreenBase const & s, uint32_t tileSize, double visualScale)
{
  return GetDrawTileScale(GetTileScaleBase(s, tileSize), tileSize, visualScale);
}

int GetDrawTileScale(m2::RectD const & r, uint32_t tileSize, double visualScale)
{
  return GetDrawTileScale(GetTileScaleBase(r), tileSize, visualScale);
}

int GetDrawTileScale(int baseScale)
{
  VisualParams const & p = VisualParams::Instance();
  return GetDrawTileScale(baseScale, p.GetTileSize(), p.GetVisualScale());
}

int GetDrawTileScale(ScreenBase const & s)
{
  VisualParams const & p = VisualParams::Instance();
  return GetDrawTileScale(s, p.GetTileSize(), p.GetVisualScale());
}

int GetDrawTileScale(m2::RectD const & r)
{
  VisualParams const & p = VisualParams::Instance();
  return GetDrawTileScale(r, p.GetTileSize(), p.GetVisualScale());
}

double GetZoomLevel(double scale)
{
  static double const kLog2 = log(2.0);
  return my::clamp(fabs(log(scale) / kLog2), 1.0, scales::GetUpperStyleScale() + 1.0);
}

void ExtractZoomFactors(ScreenBase const & s, double & zoom, int & index, float & lerpCoef)
{
  double const zoomLevel = GetZoomLevel(s.GetScale());
  zoom = trunc(zoomLevel);
  index = static_cast<int>(zoom - 1.0);
  lerpCoef = static_cast<float>(zoomLevel - zoom);
}

float InterpolateByZoomLevels(int index, float lerpCoef, std::vector<float> const & values)
{
  ASSERT_GREATER_OR_EQUAL(index, 0, ());
  ASSERT_GREATER(values.size(), scales::UPPER_STYLE_SCALE, ());
  if (index < scales::UPPER_STYLE_SCALE)
    return values[index] + lerpCoef * (values[index + 1] - values[index]);
  return values[scales::UPPER_STYLE_SCALE];
}

m2::PointF InterpolateByZoomLevels(int index, float lerpCoef, std::vector<m2::PointF> const & values)
{
  ASSERT_GREATER_OR_EQUAL(index, 0, ());
  ASSERT_GREATER(values.size(), scales::UPPER_STYLE_SCALE, ());
  if (index < scales::UPPER_STYLE_SCALE)
    return values[index] + (values[index + 1] - values[index]) * lerpCoef;
  return values[scales::UPPER_STYLE_SCALE];
}

double GetNormalizedZoomLevel(double scale, int minZoom)
{
  double const kMaxZoom = scales::GetUpperStyleScale() + 1.0;
  return my::clamp((GetZoomLevel(scale) - minZoom) / (kMaxZoom - minZoom), 0.0, 1.0);
}

double GetScale(double zoomLevel)
{
  return pow(2.0, -zoomLevel);
}

} // namespace df
